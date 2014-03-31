#ifndef __BATTERY_H__
#define __BATTERY_H__

#include "fpint.h"

/**
 * init battery functionality
 */
void battery_init();

/**
 * battery level ranging from 0 to 100
 */
int battery_level();

/**
 * capacity of battery in mAh from 0 to battery_maxcapacity()
 */
fpint battery_capacity();

/**
 * maximum capacity of battery in mAh
 */
fpint battery_maxcapacity();

#endif /* __BATTERY_H__ */
