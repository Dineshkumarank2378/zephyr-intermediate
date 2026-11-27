#ifndef SENSOR_FAKE_H
#define SENSOR_FAKE_H

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/fff.h>

DECLARE_FAKE_VALUE_FUNC(
    int,
    fake_sensor_sample_fetch,
    const struct device *,
    enum sensor_channel
);

DECLARE_FAKE_VALUE_FUNC(
    int,
    fake_sensor_channel_get,
    const struct device *,
    enum sensor_channel,
    struct sensor_value *
);

const struct device *sensor_fake_get_device(void);

void sensor_fake_reset(void);

#endif