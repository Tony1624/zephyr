
#include "pressure_sensor.h"

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pressure);

#if DT_NODE_EXISTS(DT_ALIAS(pressure_sensor))
#define PRESSURE_NODE DT_ALIAS(pressure_sensor)
const struct device *const pressure_dev = DEVICE_DT_GET(DT_ALIAS(pressure_sensor));
#else
#error("Pressure sensor not found.");
#endif

void pressure_sensor_process_sample(void)
{
    if (!device_is_ready(pressure_dev)) {
        LOG_ERR("sensor: %s device not ready.", pressure_dev->name);
        return;
    }

    if (sensor_sample_fetch(pressure_dev) < 0) {
        LOG_INF("Sensor sample update error");
        return;
    }

    struct sensor_value pressure;
    if (sensor_channel_get(pressure_dev, SENSOR_CHAN_PRESS, &pressure) < 0) {
        LOG_ERR("Cannot read pressure channel");
        return;
    }

    /* display pressure */
    LOG_INF("Pressure:%.1f kPa", sensor_value_to_double(&pressure));
}

int pressure_sensor_init(void)
{
    if (!device_is_ready(pressure_dev)) {
        LOG_ERR("sensor: %s device not ready.", pressure_dev->name);
        return -1;
    }

    pressure_sensor_process_sample();

    return 0;
}
