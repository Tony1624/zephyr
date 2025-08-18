#include "hum_temp_sensor.h"
#include "imu_sensor.h"
#include "pressure_sensor.h"

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <string.h>

LOG_MODULE_REGISTER(main);

/* Mount points */
#define MOUNT_POINT_HUM   "/lfs_hum"
#define MOUNT_POINT_PRESS "/lfs_press"
#define MOUNT_POINT_TEMP  "/lfs_temp"

/* LittleFS configs */
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(lfs_hum);
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(lfs_press);
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(lfs_temp);

static struct fs_mount_t mount_hum = {
    .type = FS_LITTLEFS,
    .fs_data = &lfs_hum,
    .storage_dev = (void *)FIXED_PARTITION_ID(humidity_fs),
    .mnt_point = MOUNT_POINT_HUM,
};

static struct fs_mount_t mount_press = {
    .type = FS_LITTLEFS,
    .fs_data = &lfs_press,
    .storage_dev = (void *)FIXED_PARTITION_ID(pressure_fs),
    .mnt_point = MOUNT_POINT_PRESS,
};

static struct fs_mount_t mount_temp = {
    .type = FS_LITTLEFS,
    .fs_data = &lfs_temp,
    .storage_dev = (void *)FIXED_PARTITION_ID(temp_fs),
    .mnt_point = MOUNT_POINT_TEMP,
};

/*--- Threads --- */
static struct k_thread hum_thread_data, press_thread_data, imu_thread_data;
static K_THREAD_STACK_DEFINE(hum_stack, 2048);
static K_THREAD_STACK_DEFINE(press_stack, 2048);
static K_THREAD_STACK_DEFINE(imu_stack, 2048);

static k_tid_t hum_tid = NULL, press_tid = NULL, imu_tid = NULL;

void hum_thread(void *a, void *b, void *c)
{
    struct fs_file_t file;
    char buffer[128];

    while (1) {
        if (hum_temp_sensor_get_string(buffer, sizeof(buffer)) > 0) {
            fs_file_t_init(&file);
            if (fs_open(&file, MOUNT_POINT_HUM "/humidity.txt",FS_O_CREATE | FS_O_WRITE | FS_O_APPEND) == 0) {
                fs_write(&file, buffer, strlen(buffer));
                fs_close(&file);
            }
        }
        k_sleep(K_SECONDS(2));
    }
}

void press_thread(void *a, void *b, void *c)
{
    struct fs_file_t file;
    char buffer[128];

    while (1) {
        if (pressure_sensor_get_string(buffer, sizeof(buffer)) > 0) {
            fs_file_t_init(&file);
            if (fs_open(&file, MOUNT_POINT_PRESS "/pressure.txt",FS_O_CREATE | FS_O_WRITE | FS_O_APPEND) == 0) {
                fs_write(&file, buffer, strlen(buffer));
                fs_close(&file);
            }
        }
        k_sleep(K_SECONDS(3));
    }
}

void imu_thread(void *a, void *b, void *c)
{
    struct fs_file_t file;
    char buffer[256];

    while (1) {
        if (imu_sensor_get_string(buffer, sizeof(buffer)) > 0) {
            fs_file_t_init(&file);
            if (fs_open(&file, MOUNT_POINT_TEMP "/imu.txt",FS_O_CREATE | FS_O_WRITE | FS_O_APPEND) == 0) {
                fs_write(&file, buffer, strlen(buffer));
                fs_close(&file);
            }
        }
        k_sleep(K_SECONDS(4));
    }
}

/* --- Shell Commands --- */
static int cmd_start_hum(const struct shell *sh, size_t argc, char **argv)
{
    if (!hum_tid) {
        hum_tid = k_thread_create(&hum_thread_data, hum_stack,K_THREAD_STACK_SIZEOF(hum_stack),hum_thread, NULL, NULL, NULL,5, 0, K_NO_WAIT);
        shell_print(sh, "Humidity logging started.");
    } else {
        shell_print(sh, "Humidity logging already running.");
    }
    return 0;
}

static int cmd_stop_hum(const struct shell *sh, size_t argc, char **argv)
{
    if (hum_tid) {
        k_thread_abort(hum_tid);
        hum_tid = NULL;
        shell_print(sh, "Humidity logging stopped.");
    } else {
        shell_print(sh, "Humidity logging not running.");
    }
    return 0;
}

static int cmd_start_press(const struct shell *sh, size_t argc, char **argv)
{
    if (!press_tid) {
        press_tid = k_thread_create(&press_thread_data, press_stack,K_THREAD_STACK_SIZEOF(press_stack),press_thread, NULL, NULL, NULL,5, 0, K_NO_WAIT);
        shell_print(sh, "Pressure logging started.");
    } else {
        shell_print(sh, "Pressure logging already running.");
    }
    return 0;
}

static int cmd_stop_press(const struct shell *sh, size_t argc, char **argv)
{
    if (press_tid) {
        k_thread_abort(press_tid);
        press_tid = NULL;
        shell_print(sh, "Pressure logging stopped.");
    } else {
        shell_print(sh, "Pressure logging not running.");
    }
    return 0;
}

static int cmd_start_imu(const struct shell *sh, size_t argc, char **argv)
{
    if (!imu_tid) {
        imu_tid = k_thread_create(&imu_thread_data, imu_stack,K_THREAD_STACK_SIZEOF(imu_stack),imu_thread, NULL, NULL, NULL,5, 0, K_NO_WAIT);
        shell_print(sh, "IMU logging started.");
    } else {
        shell_print(sh, "IMU logging already running.");
    }
    return 0;
}

static int cmd_stop_imu(const struct shell *sh, size_t argc, char **argv)
{
    if (imu_tid) {
        k_thread_abort(imu_tid);
        imu_tid = NULL;
        shell_print(sh, "IMU logging stopped.");
    } else {
        shell_print(sh, "IMU logging not running.");
    }
    return 0;
}

static int cmd_fetch_all(const struct shell *sh, size_t argc, char **argv)
{
    char buf[256];
    if (hum_temp_sensor_get_string(buf, sizeof(buf)) > 0)
        shell_print(sh, "%s", buf);
    if (pressure_sensor_get_string(buf, sizeof(buf)) > 0)
        shell_print(sh, "%s", buf);
    if (imu_sensor_get_string(buf, sizeof(buf)) > 0)
        shell_print(sh, "%s", buf);
    return 0;
}

/* Function to clear all log files */
static int cmd_clear_logs(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    int ret;

    ret = fs_unlink(MOUNT_POINT_HUM "/humidity.txt");
    if (ret < 0 && ret != -ENOENT) {
        shell_fprintf(shell, SHELL_ERROR, "Failed to remove humidity.txt (%d)\n", ret);
    }

    ret = fs_unlink(MOUNT_POINT_PRESS "/pressure.txt");
    if (ret < 0 && ret != -ENOENT) {
        shell_fprintf(shell, SHELL_ERROR, "Failed to remove pressure.txt (%d)\n", ret);
    }

    ret = fs_unlink(MOUNT_POINT_TEMP "/imu.txt");
    if (ret < 0 && ret != -ENOENT) {
        shell_fprintf(shell, SHELL_ERROR, "Failed to remove imu.txt (%d)\n", ret);
    }

    shell_fprintf(shell, SHELL_NORMAL, "All log files cleared!\n");
    return 0;
}

/* Shell command tree */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_sensors,
    SHELL_CMD(start_hum, NULL, "Start humidity logging thread", cmd_start_hum),
    SHELL_CMD(stop_hum, NULL, "Stop humidity logging thread", cmd_stop_hum),
    SHELL_CMD(start_press, NULL, "Start pressure logging thread", cmd_start_press),
    SHELL_CMD(stop_press, NULL, "Stop pressure logging thread", cmd_stop_press),
    SHELL_CMD(start_imu, NULL, "Start IMU logging thread", cmd_start_imu),
    SHELL_CMD(stop_imu, NULL, "Stop IMU logging thread", cmd_stop_imu),
    SHELL_CMD(fetch_all, NULL, "Fetch one-shot from all sensors", cmd_fetch_all),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(sensors, &sub_sensors, "Sensor logging commands", NULL);
SHELL_CMD_REGISTER(clear_logs, NULL, "Delete all sensor log files", cmd_clear_logs);

/* --- Main ---*/
static void mount_fs(struct fs_mount_t *mp)
{
    int rc = fs_mount(mp);
    if (rc == 0) {
        LOG_INF("Mounted at %s", mp->mnt_point);
    } else {
        LOG_ERR("Failed to mount %s (%d)", mp->mnt_point, rc);
    }
}

int main(void)
{
    LOG_INF("Sensor shell logging demo starting...");

    hun_temp_sensor_init();
    pressure_sensor_init();
    imu_sensor_init();

    mount_fs(&mount_hum);
    mount_fs(&mount_press);
    mount_fs(&mount_temp);

    return 0;
}
