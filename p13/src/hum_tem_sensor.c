
#include "hum_temp_sensor.h"

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

LOG_MODULE_REGISTER(hum_temp);

#if DT_NODE_EXISTS(DT_ALIAS(ht_sensor))
#define HUM_TEMP_NODE DT_ALIAS(ht_sensor)
const struct device *const hts_dev = DEVICE_DT_GET(DT_ALIAS(ht_sensor));
#else
#error("Humidity-Temperature sensor not found.");
#endif

void hum_temp_sensor_process_sample(void)
{
    if (!device_is_ready(hts_dev)) {
        LOG_ERR("sensor: %s device not ready.", hts_dev->name);
        return;
    }

    if (sensor_sample_fetch(hts_dev) < 0) {
        LOG_ERR("Sensor sample update error");
        return;
    }

    struct sensor_value temp, hum;
    if (sensor_channel_get(hts_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
        LOG_ERR("Cannot read HTS221 temperature channel");
        return;
    }

    if (sensor_channel_get(hts_dev, SENSOR_CHAN_HUMIDITY, &hum) < 0) {
        LOG_ERR("Cannot read HTS221 humidity channel");
        return;
    }

    /* display temperature */
    LOG_INF("Temperature:%.1f C", sensor_value_to_double(&temp));

    /* display humidity */
    LOG_INF("Relative Humidity:%.1f%%", sensor_value_to_double(&hum));
}

int hun_temp_sensor_init(void)
{
    if (!device_is_ready(hts_dev)) {
        LOG_ERR("sensor: %s device not ready.", hts_dev->name);
        return -1;
    }

    hum_temp_sensor_process_sample();

    return 0;
}
