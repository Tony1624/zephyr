#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <string.h>
#include <zephyr/shell/shell.h>

#include "sensors_common.h"
#include "hum_temp_sensor.h"
#include "pressure_sensor.h"
#include "imu_sensor.h"

/* -------- Logging -------- */
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

/* Mount point & partition */
static int littlefs_mount(void);
static int log_open(void);
static int log_write_snapshot(const struct all_sensors_data *d);

/* Avoid symbol clash with littlefs function lfs_mount() */
static struct fs_mount_t lfs_mnt;

/* LittleFS backing storage descriptor */
static struct fs_file_t log_file;

#define LOG_MOUNT_POINT   "/lfs"
#define LOG_FILE_PATH     LOG_MOUNT_POINT "/sensor_log.bin"

/* Circular logging parameters */
#define LOG_HEADER_MAGIC  0x53454E31u /* 'SEN1' */
struct log_header {
    uint32_t magic;
    uint32_t write_off; /* offset within payload area (not counting header) */
};

/* Upper bound for file payload; keep conservative inside 128KB partition */
#define LOG_PAYLOAD_MAX   (64 * 1024) /* 64KB for payload */
#define LOG_HEADER_SIZE   (sizeof(struct log_header))

/* -------- IPC: message queue -------- */
K_MSGQ_DEFINE(sensor_q, sizeof(struct all_sensors_data), 32, 4);

/* Shared latest snapshot guarded by mutex */
static struct all_sensors_data g_last;
static struct k_mutex g_last_lock;

/* -------- Threads & stacks -------- */
#define STACK_SZ 2048
K_THREAD_STACK_DEFINE(ht_stack,    STACK_SZ);
K_THREAD_STACK_DEFINE(press_stack, STACK_SZ);
K_THREAD_STACK_DEFINE(imu_stack,   STACK_SZ);
K_THREAD_STACK_DEFINE(log_stack,   STACK_SZ);

static struct k_thread ht_thread_data;
static struct k_thread press_thread_data;
static struct k_thread imu_thread_data;
static struct k_thread log_thread_data;

/* -------- Sensor producer threads -------- */
static void ht_thread(void *, void *, void *)
{
    (void)hum_temp_sensor_init();
    while (1) {
        int16_t t=0, h=0;
        if (hum_temp_sensor_fetch(&t, &h) == 0) {
            struct all_sensors_data snap;

            k_mutex_lock(&g_last_lock, K_FOREVER);
            /* start from last, update our fields */
            snap = g_last;
            snap.ht.temperature = t;
            snap.ht.humidity    = h;
            g_last = snap;
            k_mutex_unlock(&g_last_lock);

            /* try to queue; if full, drop oldest then put */
            if (k_msgq_put(&sensor_q, &snap, K_NO_WAIT) != 0) {
                struct all_sensors_data trash;
                k_msgq_get(&sensor_q, &trash, K_NO_WAIT);
                k_msgq_put(&sensor_q, &snap, K_NO_WAIT);
            }

            /* minimal UART prints (constant strings only) */
            printk("HT T=");
            printk("%d", (int)snap.ht.temperature);
            printk(" H=");
            printk("%d\n", (int)snap.ht.humidity);
        }
        k_sleep(K_MSEC(500));
    }
}

static void press_thread(void *, void *, void *)
{
    (void)pressure_sensor_init();
    while (1) {
        int32_t p=0;
        if (pressure_sensor_fetch(&p) == 0) {
            struct all_sensors_data snap;

            k_mutex_lock(&g_last_lock, K_FOREVER);
            snap = g_last;
            snap.press.pressure = p;
            g_last = snap;
            k_mutex_unlock(&g_last_lock);

            if (k_msgq_put(&sensor_q, &snap, K_NO_WAIT) != 0) {
                struct all_sensors_data trash;
                k_msgq_get(&sensor_q, &trash, K_NO_WAIT);
                k_msgq_put(&sensor_q, &snap, K_NO_WAIT);
            }

            printk("P ");
            printk("%d\n", (int)snap.press.pressure);
        }
        k_sleep(K_MSEC(500));
    }
}

static void imu_thread(void *, void *, void *)
{
    (void)imu_sensor_init();
    while (1) {
        int16_t ax=0, ay=0, az=0, gx=0, gy=0, gz=0;
        if (imu_sensor_fetch(&ax, &ay, &az, &gx, &gy, &gz) == 0) {
            struct all_sensors_data snap;

            k_mutex_lock(&g_last_lock, K_FOREVER);
            snap = g_last;
            snap.imu.accel.x = ax;
            snap.imu.accel.y = ay;
            snap.imu.accel.z = az;
            snap.imu.gyro.x  = gx;
            snap.imu.gyro.y  = gy;
            snap.imu.gyro.z  = gz;
            g_last = snap;
            k_mutex_unlock(&g_last_lock);

            if (k_msgq_put(&sensor_q, &snap, K_NO_WAIT) != 0) {
                struct all_sensors_data trash;
                k_msgq_get(&sensor_q, &trash, K_NO_WAIT);
                k_msgq_put(&sensor_q, &snap, K_NO_WAIT);
            }

            printk("IMU A:");
            printk("%d", (int)snap.imu.accel.x); printk(",");
            printk("%d", (int)snap.imu.accel.y); printk(",");
            printk("%d", (int)snap.imu.accel.z); printk(" G:");
            printk("%d", (int)snap.imu.gyro.x);  printk(",");
            printk("%d", (int)snap.imu.gyro.y);  printk(",");
            printk("%d\n", (int)snap.imu.gyro.z);
        }
        k_sleep(K_MSEC(500));
    }
}

/* -------- Logger consumer thread -------- */
static void log_thread(void *, void *, void *)
{
    if (littlefs_mount() != 0) {
        LOG_ERR("FS mount failed");
        return;
    }
    if (log_open() != 0) {
        LOG_ERR("Open log failed");
        return;
    }

    while (1) {
        struct all_sensors_data d;
        if (k_msgq_get(&sensor_q, &d, K_FOREVER) == 0) {
            int rc = log_write_snapshot(&d);
            if (rc) {
                LOG_ERR("log write err %d", rc);
                /* backoff a bit on error */
                k_sleep(K_MSEC(1000));
            }
        }
    }
}

/* -------- LittleFS helpers -------- */
static int littlefs_mount(void)
{
    /* Storage device: use the fixed partition from DTS */
    static struct fs_littlefs lfs_data;

    lfs_mnt.type = FS_LITTLEFS;
    lfs_mnt.fs_data = &lfs_data;
    lfs_mnt.storage_dev = (void *)FIXED_PARTITION_ID(logs_fs);
    lfs_mnt.mnt_point = LOG_MOUNT_POINT;

    int rc = fs_mount(&lfs_mnt);
    if (rc == 0) {
        LOG_INF("Mounted at %s", LOG_MOUNT_POINT);
    }
    return rc;
}

static int read_header(struct log_header *hdr)
{
    int rc = fs_seek(&log_file, 0, FS_SEEK_SET);
    if (rc) return rc;

    ssize_t r = fs_read(&log_file, hdr, sizeof(*hdr));
    if (r < 0) return (int)r;
    if (r != sizeof(*hdr)) return -EIO;

    if (hdr->magic != LOG_HEADER_MAGIC) return -EINVAL;
    if (hdr->write_off >= LOG_PAYLOAD_MAX) return -EINVAL;
    return 0;
}

static int write_header(const struct log_header *hdr)
{
    int rc = fs_seek(&log_file, 0, FS_SEEK_SET);
    if (rc) return rc;
    ssize_t w = fs_write(&log_file, hdr, sizeof(*hdr));
    return (w == sizeof(*hdr)) ? 0 : -EIO;
}

static int log_open(void)
{
    fs_file_t_init(&log_file);
    int rc = fs_open(&log_file, LOG_FILE_PATH, FS_O_CREATE | FS_O_RDWR);
    if (rc) return rc;

    struct log_header hdr;
    rc = read_header(&hdr);
    if (rc) {
        /* initialize */
        hdr.magic = LOG_HEADER_MAGIC;
        hdr.write_off = 0;

        /* Preallocate payload area with 0xFF */
        rc = fs_seek(&log_file, LOG_HEADER_SIZE, FS_SEEK_SET);
        if (rc) return rc;

        uint8_t buf[64];
        memset(buf, 0xFF, sizeof(buf));
        size_t total = 0;
        while (total < LOG_PAYLOAD_MAX) {
            size_t chunk = MIN(sizeof(buf), LOG_PAYLOAD_MAX - total);
            ssize_t w = fs_write(&log_file, buf, chunk);
            if (w != chunk) return -EIO;
            total += w;
        }

        /* write header at start */
        rc = write_header(&hdr);
        if (rc) return rc;
    }
    return 0;
}


static int log_write_snapshot(const struct all_sensors_data *d)
{
    struct log_header hdr;
    int rc = read_header(&hdr);
    if (rc) return rc;

    /* Where to write: header + write_off */
    off_t pos = LOG_HEADER_SIZE + hdr.write_off;

    rc = fs_seek(&log_file, pos, FS_SEEK_SET);
    if (rc) return rc;

    ssize_t w = fs_write(&log_file, d, sizeof(*d));
    if (w != sizeof(*d)) return -EIO;

    /* advance write_off with wrap */
    uint32_t next = hdr.write_off + sizeof(*d);
    if (next + sizeof(*d) > LOG_PAYLOAD_MAX) {
        next = 0; /* wrap to start of payload */
    }
    hdr.write_off = next;

    return write_header(&hdr);
}
static int cmd_clear_logs(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    int ret = fs_unlink(LOG_FILE_PATH);
    if (ret == -ENOENT) {
        shell_print(shell, "No log file to delete.");
    } else if (ret) {
        shell_error(shell, "Failed to delete log file (%d)", ret);
        return ret;
    } else {
        shell_print(shell, "Log file deleted.");
    }
    return 0;
}

SHELL_CMD_REGISTER(clear_logs, NULL, "Delete sensor_log.bin file", cmd_clear_logs);
/* -------- main -------- */
void main(void)
{
    k_mutex_init(&g_last_lock);
    memset(&g_last, 0, sizeof(g_last));

    /* Start producers */
    k_thread_create(&ht_thread_data, ht_stack, K_THREAD_STACK_SIZEOF(ht_stack),
                    ht_thread, NULL, NULL, NULL, 5, 0, K_NO_WAIT);

    k_thread_create(&press_thread_data, press_stack, K_THREAD_STACK_SIZEOF(press_stack),
                    press_thread, NULL, NULL, NULL, 5, 0, K_NO_WAIT);

    k_thread_create(&imu_thread_data, imu_stack, K_THREAD_STACK_SIZEOF(imu_stack),
                    imu_thread, NULL, NULL, NULL, 5, 0, K_NO_WAIT);

    /* Start logger */
    k_thread_create(&log_thread_data, log_stack, K_THREAD_STACK_SIZEOF(log_stack),
                    log_thread, NULL, NULL, NULL, 6, 0, K_NO_WAIT);
}
