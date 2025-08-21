#include "pressure_sensor.h"
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

LOG_MODULE_REGISTER(pressure);

#if DT_NODE_EXISTS(DT_ALIAS(pressure_sensor))
#define PRESSURE_NODE DT_ALIAS(pressure_sensor)
const struct device *const pressure_dev = DEVICE_DT_GET(DT_ALIAS(pressure_sensor));
#else
#error("Pressure sensor not found.");
#endif

int pressure_sensor_get_string(char *buf, size_t buf_len)
{
    if (!device_is_ready(pressure_dev)) return -1;
    if (sensor_sample_fetch(pressure_dev) < 0) return -1;

    struct sensor_value pressure;
    if (sensor_channel_get(pressure_dev, SENSOR_CHAN_PRESS, &pressure) < 0) return -1;

    double p = sensor_value_to_double(&pressure);
   LOG_INF("Pressure: %.1f kPa", p);

    return snprintf(buf, buf_len, "Pressure: %.1f kPa\n", p);
}

int pressure_sensor_init(void)
{
    if (!device_is_ready(pressure_dev)) {
        LOG_ERR("sensor: %s device not ready.", pressure_dev->name);
        return -1;
    }
    return 0;
}
