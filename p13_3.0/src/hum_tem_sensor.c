#include "hum_temp_sensor.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

/* alias in overlay: ht-sensor */
#define HT_NODE DT_ALIAS(ht_sensor)
#if !DT_NODE_HAS_STATUS(HT_NODE, okay)
#error "HTS221 not enabled in devicetree"
#endif

static const struct device *ht_dev;

int hum_temp_sensor_init(void)
{
    ht_dev = DEVICE_DT_GET(HT_NODE);
    return device_is_ready(ht_dev) ? 0 : -ENODEV;
}

int hum_temp_sensor_fetch(int16_t *temp, int16_t *hum)
{
    if (!ht_dev) return -ENODEV;

    int rc = sensor_sample_fetch(ht_dev);
    if (rc) return rc;

    struct sensor_value t, h;
    rc = sensor_channel_get(ht_dev, SENSOR_CHAN_AMBIENT_TEMP, &t);
    if (rc) return rc;
    rc = sensor_channel_get(ht_dev, SENSOR_CHAN_HUMIDITY, &h);
    if (rc) return rc;

    /* Convert to fixed-point small ints: 0.01 units */
    /* sensor_value: val1 integer, val2 micro */
    int32_t t_centi = (int32_t)t.val1 * 100 + t.val2 / 10000; /* micro to centi */
    int32_t h_centi = (int32_t)h.val1 * 100 + h.val2 / 10000;

    if (temp) *temp = (int16_t)t_centi;
    if (hum)  *hum  = (int16_t)h_centi;

    return 0;
}
