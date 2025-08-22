#ifndef HUM_TEMP_SENSOR_H
#define HUM_TEMP_SENSOR_H

#include <stdint.h>

int hum_temp_sensor_init(void);
/* returns 0 on success; temp in 0.01°C, hum in 0.01%RH */
int hum_temp_sensor_fetch(int16_t *temp, int16_t *hum);

#endif
