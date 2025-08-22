#include "imu_sensor.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

/* alias in overlay: imu-sensor */
#define IMU_NODE DT_ALIAS(imu_sensor)
#if !DT_NODE_HAS_STATUS(IMU_NODE, okay)
#error "LSM6DSL not enabled in devicetree"
#endif

static const struct device *imu_dev;

int imu_sensor_init(void)
{
    imu_dev = DEVICE_DT_GET(IMU_NODE);
    return device_is_ready(imu_dev) ? 0 : -ENODEV;
}

int imu_sensor_fetch(int16_t *ax, int16_t *ay, int16_t *az,
                     int16_t *gx, int16_t *gy, int16_t *gz)
{
    if (!imu_dev) return -ENODEV;

    int rc = sensor_sample_fetch(imu_dev);
    if (rc) return rc;

    struct sensor_value av[3], gv[3];

    rc = sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, av);
    if (rc) return rc;
    rc = sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, gv);
    if (rc) return rc;

    /* Convert to small fixed ints: e.g., mg and mdps */
    /* sensor_value is SI units; we choose centi-units for accel (m/s^2*100) and
       mdps for gyro: deg/s * 1000 (approx via micro). Adjust as needed. */

    if (ax) *ax = (int16_t)(av[0].val1 * 100 + av[0].val2 / 10000);
    if (ay) *ay = (int16_t)(av[1].val1 * 100 + av[1].val2 / 10000);
    if (az) *az = (int16_t)(av[2].val1 * 100 + av[2].val2 / 10000);

    if (gx) *gx = (int16_t)(gv[0].val1 * 100 + gv[0].val2 / 10000);
    if (gy) *gy = (int16_t)(gv[1].val1 * 100 + gv[1].val2 / 10000);
    if (gz) *gz = (int16_t)(gv[2].val1 * 100 + gv[2].val2 / 10000);

    return 0;
}
