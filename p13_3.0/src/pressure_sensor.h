#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

#include <stdint.h>

int pressure_sensor_init(void);
/* returns 0 on success; press in Pa */
int pressure_sensor_fetch(int32_t *press);

#endif
