#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H
#include <stddef.h>

int pressure_sensor_init(void);
/* New */
int pressure_sensor_get_string(char *buf, size_t buf_len);

#endif /* PRESSURE_SENSOR_H */
