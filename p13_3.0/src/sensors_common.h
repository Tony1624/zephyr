#ifndef SENSORS_COMMON_H
#define SENSORS_COMMON_H

#include <stdint.h>

/* Master struct that holds everything (your layout) */
struct all_sensors_data {
    /* HTS221 */
    struct {
        int16_t temperature;  /* e.g. degC * 100 */
        int16_t humidity;     /* e.g. %RH * 100 */
    } ht;

    /* LPS22HB */
    struct {
        int32_t pressure;     /* e.g. Pa */
    } press;

    /* LSM6DSL (IMU) */
    struct {
        struct {
            int16_t x;
            int16_t y;
            int16_t z;
        } accel;

        struct {
            int16_t x;
            int16_t y;
            int16_t z;
        } gyro;
    } imu;
};

#endif /* SENSORS_COMMON_H */
