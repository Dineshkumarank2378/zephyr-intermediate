#include "temp_alarm.h"

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(temp_alarm, LOG_LEVEL_INF);

static const struct device *sensor_dev;

static bool alarm_active;

int temp_alarm_init(const struct device *sensor)
{
    if (sensor == NULL) {
        return -EINVAL;
    }

    if (!device_is_ready(sensor)) {
        return -ENODEV;
    }

    sensor_dev = sensor;
    alarm_active = false;

    LOG_INF("Temperature alarm initialized");

    return 0;
}

int temp_alarm_check(void)
{
    struct sensor_value temperature;
    double temperature_c;
    int ret;

    if (sensor_dev == NULL) {
        return -EINVAL;
    }

    /* Fetch a new sensor sample */
    ret = sensor_sample_fetch_chan(sensor_dev,
                                   SENSOR_CHAN_AMBIENT_TEMP);

    if (ret != 0) {
        LOG_ERR("Sensor fetch failed: %d", ret);

        return ret;
    }

    /* Read the fetched temperature */
    ret = sensor_channel_get(sensor_dev,
                             SENSOR_CHAN_AMBIENT_TEMP,
                             &temperature);

    if (ret != 0) {
        LOG_ERR("Temperature read failed: %d", ret);

        return ret;
    }

    temperature_c = sensor_value_to_double(&temperature);

    LOG_INF("Temperature: %.2f C", temperature_c);

    if (temperature_c > TEMP_ALARM_THRESHOLD_C) {
        alarm_active = true;

        LOG_WRN("Temperature alarm activated!");
    } else {
        alarm_active = false;

        LOG_INF("Temperature is normal");
    }

    return 0;
}

bool temp_alarm_is_active(void)
{
    return alarm_active;
}

void temp_alarm_reset(void)
{
    alarm_active = false;

    LOG_INF("Temperature alarm reset");
}