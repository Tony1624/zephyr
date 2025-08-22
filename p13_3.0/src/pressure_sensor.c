#include "pressure_sensor.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

/* alias in overlay: pressure-sensor */
#define PRESS_NODE DT_ALIAS(pressure_sensor)
#if !DT_NODE_HAS_STATUS(PRESS_NODE, okay)
#error "LPS22HB not enabled in devicetree"
#endif

static const struct device *press_dev;

int pressure_sensor_init(void)
{
    press_dev = DEVICE_DT_GET(PRESS_NODE);
    return device_is_ready(press_dev) ? 0 : -ENODEV;
}

int pressure_sensor_fetch(int32_t *press)
{
    if (!press_dev) return -ENODEV;

    int rc = sensor_sample_fetch(press_dev);
    if (rc) return rc;

    struct sensor_value p;
    rc = sensor_channel_get(press_dev, SENSOR_CHAN_PRESS, &p);
    if (rc) return rc;

    /* Pa: val1 in kPa? No—Zephyr uses SI units.
       SENSOR_CHAN_PRESS is in kPa in many drivers; convert to Pa carefully.
       Commonly: p.val1 = kPa integer, p.val2 = kPa micro.
       Convert to Pa: (val1*1000 + val2/1000) */
    int64_t pa = (int64_t)p.val1 * 1000 + p.val2 / 1000;
    if (press) *press = (int32_t)pa;
    return 0;
}
