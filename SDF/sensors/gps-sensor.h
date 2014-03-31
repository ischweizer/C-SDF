#ifndef __GPS_SENSOR_H__
#define __GPS_SENSOR_H__

#include "fpint.h"

typedef struct {
    fpint latitude;
    fpint longitude;
} gps_position;

/**
 * reads value from gps sensor
 */
void gps_value(gps_position* position);

#endif /* __GPS_SENSOR_H__ */
