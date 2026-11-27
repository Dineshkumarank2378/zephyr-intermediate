#include "sensor_fake.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sensor_fake, LOG_LEVEL_INF);

DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(
    int,
    fake_sensor_sample_fetch,
    const struct device *,
    enum sensor_channel
);

DEFINE_FAKE_VALUE_FUNC(
    int,
    fake_sensor_channel_get,
    const struct device *,
    enum sensor_channel,
    struct sensor_value *
);

/*
 * Connect FFF fake functions to the Zephyr sensor API.
 */

static const struct sensor_driver_api fake_sensor_api = {
    .sample_fetch = fake_sensor_sample_fetch,
    .channel_get = fake_sensor_channel_get,
};

/*
 * Zephyr initializes this device during boot.
 */

static int fake_sensor_init(const struct device *dev)
{
    ARG_UNUSED(dev);

    return 0;
}

DEVICE_DEFINE(
    fake_sensor,
    "FAKE_SENSOR",
    fake_sensor_init,
    NULL,
    NULL,
    NULL,
    POST_KERNEL,
    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
    &fake_sensor_api
);

const struct device *sensor_fake_get_device(void)
{
    return DEVICE_GET(fake_sensor);
}

void sensor_fake_reset(void)
{
    RESET_FAKE(fake_sensor_sample_fetch);
    RESET_FAKE(fake_sensor_channel_get);

    FFF_RESET_HISTORY();
}