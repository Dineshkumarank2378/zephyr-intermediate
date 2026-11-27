#ifndef TEMP_ALARM_H
#define TEMP_ALARM_H

#include <zephyr/device.h>
#include <stdbool.h>

#define TEMP_ALARM_THRESHOLD_C 30.0

int temp_alarm_init(const struct device *sensor);

int temp_alarm_check(void);

bool temp_alarm_is_active(void);

void temp_alarm_reset(void);

#endif