#include "hum_temp_sensor.h"
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

LOG_MODULE_REGISTER(hum_temp);

#if DT_NODE_EXISTS(DT_ALIAS(ht_sensor))
#define HUM_TEMP_NODE DT_ALIAS(ht_sensor)
const struct device *const hts_dev = DEVICE_DT_GET(DT_ALIAS(ht_sensor));
#else
#error("Humidity-Temperature sensor not found.");
#endif

int hum_temp_sensor_get_string(char *buf, size_t buf_len)
{
    if (!device_is_ready(hts_dev)) return -1;
    if (sensor_sample_fetch(hts_dev) < 0) return -1;

    struct sensor_value temp, hum;
    if (sensor_channel_get(hts_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) return -1;
    if (sensor_channel_get(hts_dev, SENSOR_CHAN_HUMIDITY, &hum) < 0) return -1;

    double t = sensor_value_to_double(&temp);
    double h = sensor_value_to_double(&hum);

    LOG_INF("Temperature: %.1f C", t);
    LOG_INF("Humidity: %.1f %%", h);

    return snprintf(buf, buf_len, "Temperature: %.1f C, Humidity: %.1f %%\n", t, h);
}

int hun_temp_sensor_init(void)
{
    if (!device_is_ready(hts_dev)) {
        LOG_ERR("sensor: %s device not ready.", hts_dev->name);
        return -1;
    }
    return 0;
}
