#include "hum_temp_sensor.h"
#include "imu_sensor.h"
#include "pressure_sensor.h"

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(main);

// Mount points
#define MOUNT_POINT_HUM "/lfs_hum"
#define MOUNT_POINT_PRESS "/lfs_press"
#define MOUNT_POINT_TEMP "/lfs_temp"

// LittleFS configs
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

static void mount_fs(struct fs_mount_t *mp)
{
    int rc = fs_mount(mp);
    if (rc == 0) {
        LOG_INF("Mounted at %s", mp->mnt_point);
    } else {
        LOG_ERR("Failed to mount %s (%d)", mp->mnt_point, rc);
    }
}

// ------------ Threads --------------
void hum_thread(void *a, void *b, void *c)
{
    struct fs_file_t file;
    char buffer[128];

    while (1) {
        if (hum_temp_sensor_get_string(buffer, sizeof(buffer)) > 0) {
            fs_file_t_init(&file);
            if (fs_open(&file, MOUNT_POINT_HUM "/humidity.txt", FS_O_CREATE | FS_O_WRITE | FS_O_APPEND) == 0) {
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
            if (fs_open(&file, MOUNT_POINT_PRESS "/pressure.txt", FS_O_CREATE | FS_O_WRITE | FS_O_APPEND) == 0) {
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
            if (fs_open(&file, MOUNT_POINT_TEMP "/imu.txt", FS_O_CREATE | FS_O_WRITE | FS_O_APPEND) == 0) {
                fs_write(&file, buffer, strlen(buffer));
                fs_close(&file);
            }
        }
        k_sleep(K_SECONDS(4));
    }
}

// ------------ Main -----------------
int main(void)
{
    LOG_INF("Sensor logging demo starting...");

    // Init sensors
    hun_temp_sensor_init();
    pressure_sensor_init();
    imu_sensor_init();

    // Mount FS
    mount_fs(&mount_hum);
    mount_fs(&mount_press);
    mount_fs(&mount_temp);

    // Start threads
    static K_THREAD_STACK_DEFINE(hum_stack, 2048);
    static K_THREAD_STACK_DEFINE(press_stack, 2048);
    static K_THREAD_STACK_DEFINE(imu_stack, 2048);

    static struct k_thread hum_thread_data, press_thread_data, imu_thread_data;

    k_thread_create(&hum_thread_data, hum_stack, K_THREAD_STACK_SIZEOF(hum_stack),
                    hum_thread, NULL, NULL, NULL, 5, 0, K_NO_WAIT);

    k_thread_create(&press_thread_data, press_stack, K_THREAD_STACK_SIZEOF(press_stack),
                    press_thread, NULL, NULL, NULL, 5, 0, K_NO_WAIT);

    k_thread_create(&imu_thread_data, imu_stack, K_THREAD_STACK_SIZEOF(imu_stack),
                    imu_thread, NULL, NULL, NULL, 5, 0, K_NO_WAIT);

    return 0;
}
