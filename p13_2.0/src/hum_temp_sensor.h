#ifndef HUM_TEMP_SENSOR_H
#define HUM_TEMP_SENSOR_H
#include <stddef.h>
int hun_temp_sensor_init(void);
/* New: get formatted sensor data string */
int hum_temp_sensor_get_string(char *buf, size_t buf_len);

#endif /* HUM_TEMP_SENSOR_H */
