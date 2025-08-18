#include "imu_sensor.h"
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

LOG_MODULE_REGISTER(imu);

#if DT_NODE_EXISTS(DT_ALIAS(imu_sensor))
#define IMU_NODE DT_ALIAS(imu_sensor)
const struct device *const imu_dev = DEVICE_DT_GET(DT_ALIAS(imu_sensor));
#else
#error("IMU sensor not found.");
#endif

int imu_sensor_get_string(char *buf, size_t buf_len)
{
    if (!device_is_ready(imu_dev)) return -1;
    if (sensor_sample_fetch(imu_dev) < 0) return -1;

    struct sensor_value ax, ay, az, gx, gy, gz;

    sensor_sample_fetch_chan(imu_dev, SENSOR_CHAN_ACCEL_XYZ);
    sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_X, &ax);
    sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_Y, &ay);
    sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_Z, &az);

    sensor_sample_fetch_chan(imu_dev, SENSOR_CHAN_GYRO_XYZ);
    sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_X, &gx);
    sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_Y, &gy);
    sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_Z, &gz);

    double axd = sensor_value_to_double(&ax);
    double ayd = sensor_value_to_double(&ay);
    double azd = sensor_value_to_double(&az);
    double gxd = sensor_value_to_double(&gx);
    double gyd = sensor_value_to_double(&gy);
    double gzd = sensor_value_to_double(&gz);

   LOG_INF("Accel: x=%.2f y=%.2f z=%.2f", axd, ayd, azd);
   LOG_INF("Gyro : x=%.2f y=%.2f z=%.2f", gxd, gyd, gzd);

    return snprintf(buf, buf_len,"Accel: %.2f, %.2f, %.2f | Gyro: %.2f, %.2f, %.2f\n",axd, ayd, azd, gxd, gyd, gzd);
}

int imu_sensor_init(void)
{
    if (!device_is_ready(imu_dev)) {
        LOG_ERR("IMU sensor not ready.");
        return -1;
    }
    return 0;
}
