
#include "hum_temp_sensor.h"
#include "imu_sensor.h"
#include "pressure_sensor.h"

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <stdbool.h>

K_THREAD_STACK_DEFINE(sensor_thread_stack_area, 2048);
static struct k_thread sensor_thread_data;
static k_tid_t sensor_tid;
static bool terminate_sensor_thread = false;

LOG_MODULE_REGISTER(main);

int main(void)
{
    int ret;
    ret = hun_temp_sensor_init();
    if (ret < 0) {
        LOG_ERR("Humidity-Temperature Sensor init failed");
        return ret;
    }
    ret = pressure_sensor_init();
    if (ret < 0) {
        LOG_ERR("Pressure Sensor init failed");
        return ret;
    }
    ret = imu_sensor_init();
    if (ret < 0) {
        LOG_ERR("IMU Sensor init failed");
        return ret;
    }
    return 0;
}

static void sensor_demo_thread(void *arg0, void *arg1, void *arg2)
{
    LOG_INF("sensor_demo_thread started.");

    while (!terminate_sensor_thread) {
        hum_temp_sensor_process_sample();
        pressure_sensor_process_sample();
        imu_sensor_sample_process();
        k_sleep(K_MSEC(2000));
    }

    terminate_sensor_thread = false;
    LOG_INF("sensor_demo_thread stopped.");
}

static int shell_fetch_sample(const struct shell *sh, size_t argc, char **argv, void *data)
{
    hum_temp_sensor_process_sample();
    pressure_sensor_process_sample();
    imu_sensor_sample_process();
    return 0;
}

static int shell_start_thread(const struct shell *sh, size_t argc, char **argv, void *data)
{
    sensor_tid =
        k_thread_create(&sensor_thread_data, sensor_thread_stack_area, K_THREAD_STACK_SIZEOF(sensor_thread_stack_area),
                        sensor_demo_thread, NULL, NULL, NULL, 5, 0, K_NO_WAIT);
    return 0;
}

static int shell_stop_thread(const struct shell *sh, size_t argc, char **argv, void *data)
{
    terminate_sensor_thread = true;
    k_thread_join(sensor_tid, K_FOREVER);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_demo, SHELL_CMD(fetch_sample, NULL, "Fetch sample.", shell_fetch_sample),
                               SHELL_CMD(start, NULL, "Start Sample Thread.", shell_start_thread),
                               SHELL_CMD(stop, NULL, "Stop Sample Thread.", shell_stop_thread), SHELL_SUBCMD_SET_END);
/* Creating root (level 0) command "demo" */
SHELL_CMD_REGISTER(sensor_demo, &sub_demo, "Sensor Demo commands", NULL);
