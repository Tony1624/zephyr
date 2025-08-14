
#include "imu_sensor.h"

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>

#include <stdbool.h>

LOG_MODULE_REGISTER(imu);

#if DT_NODE_EXISTS(DT_ALIAS(imu_sensor))
#define HUM_TEMP_NODE DT_ALIAS(imu_sensor)
const struct device *const imu_dev = DEVICE_DT_GET(DT_ALIAS(imu_sensor));
#else
#error("IMU sensor not found.");
#endif

void imu_sensor_sample_process(void)
{
    if (sensor_sample_fetch(imu_dev) < 0) {
        LOG_ERR("Sensor sample update error");
        return;
    }
    char out_str[64];
    struct sensor_value accel_x, accel_y, accel_z;
    struct sensor_value gyro_x, gyro_y, gyro_z;

    /* lsm6dsl accel */
    sensor_sample_fetch_chan(imu_dev, SENSOR_CHAN_ACCEL_XYZ);
    sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_X, &accel_x);
    sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_Y, &accel_y);
    sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_Z, &accel_z);

    sprintf(out_str, "accel x:%f ms/2 y:%f ms/2 z:%f ms/2", sensor_value_to_double(&accel_x),
            sensor_value_to_double(&accel_y), sensor_value_to_double(&accel_z));
    LOG_INF("%s", out_str);

    /* lsm6dsl gyro */
    sensor_sample_fetch_chan(imu_dev, SENSOR_CHAN_GYRO_XYZ);
    sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_X, &gyro_x);
    sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_Y, &gyro_y);
    sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_Z, &gyro_z);

    sprintf(out_str, "gyro x:%f dps y:%f dps z:%f dps", sensor_value_to_double(&gyro_x),
            sensor_value_to_double(&gyro_y), sensor_value_to_double(&gyro_z));
    LOG_INF("%s", out_str);
}

int imu_sensor_init(void)
{
    struct sensor_value odr_attr;
    if (!device_is_ready(imu_dev)) {
        LOG_ERR("sensor: device not ready.");
        return -1;
    }

    /* set accel/gyro sampling frequency to 104 Hz */
    odr_attr.val1 = 104;
    odr_attr.val2 = 0;

    if (sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
        LOG_ERR("Cannot set sampling frequency for accelerometer.");
        return -1;
    }

    if (sensor_attr_set(imu_dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
        LOG_ERR("Cannot set sampling frequency for gyro.");
        return -1;
    }

    imu_sensor_sample_process();
    return 0;
}
