#ifndef IMU_SENSOR_H
#define IMU_SENSOR_H
#include <stddef.h>
int imu_sensor_init(void);
/* New */
int imu_sensor_get_string(char *buf, size_t buf_len);

#endif /* IMU_SENSOR_H */
