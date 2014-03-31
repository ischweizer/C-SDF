#include "contiki.h"

#include "sdf-config.h"
#include "time.h"

unsigned long time() {
	return clock_seconds() * SPEEDMULTIPLIER;
}

unsigned int time_day() {
	unsigned long minutes_of_days = time() / 60 + TIME_MINUTE;
	return (minutes_of_days / 1440 + TIME_DAY) % 365;
}

unsigned int time_hour() {
	return time_minute() / 60;
}

unsigned int time_minute() {
	return (time() / 60 + TIME_MINUTE) % 1440;
}

unsigned int time_seconds() {
	return time() % 86400;
}
