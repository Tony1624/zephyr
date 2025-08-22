#ifndef IMU_SENSOR_H
#define IMU_SENSOR_H

#include <stdint.h>

int imu_sensor_init(void);
/* returns 0 on success; raw milli-units (e.g., mg, mdps) or scaled small ints */
int imu_sensor_fetch(int16_t *ax, int16_t *ay, int16_t *az,
                     int16_t *gx, int16_t *gy, int16_t *gz);

#endif
