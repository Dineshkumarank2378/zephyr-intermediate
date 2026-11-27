#include <zephyr/ztest.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/fff.h>

#include "temp_alarm.h"
#include "sensor_fake.h"

static struct sensor_value fake_temperature;

/*
 * Fake sensor returns the configured temperature.
 */

static int fake_get_temperature(
    const struct device *dev,
    enum sensor_channel channel,
    struct sensor_value *value)
{
    ARG_UNUSED(dev);

    if (channel != SENSOR_CHAN_AMBIENT_TEMP) {
        return -EINVAL;
    }

    *value = fake_temperature;

    return 0;
}

/*
 * Runs before every test.
 */

static void before_test(void)
{
    sensor_fake_reset();

    fake_temperature.val1 = 25;
    fake_temperature.val2 = 0;

    fake_sensor_sample_fetch.return_val = 0;

    fake_sensor_channel_get.custom_fake =
        fake_get_temperature;

    temp_alarm_reset();

    zassert_ok(
        temp_alarm_init(sensor_fake_get_device()),
        "Temperature alarm initialization failed"
    );
}

ZTEST_SUITE(
    temp_alarm,
    NULL,
    NULL,
    before_test,
    NULL,
    NULL
);

/*
 * TASK 1: Temperature below threshold
 */

ZTEST(temp_alarm, test_temperature_below_threshold)
{
    fake_temperature.val1 = 20;
    fake_temperature.val2 = 0;

    int ret = temp_alarm_check();

    zassert_equal(ret, 0,
                  "Temperature check should succeed");

    zassert_false(temp_alarm_is_active(),
                  "Alarm must remain inactive");

    zassert_equal(fake_sensor_sample_fetch.call_count, 1,
                  "Sensor must be fetched once");

    zassert_equal(fake_sensor_channel_get.call_count, 1,
                  "Temperature must be read once");
}

/*
 * TASK 1: Temperature above threshold
 */

ZTEST(temp_alarm, test_temperature_above_threshold)
{
    fake_temperature.val1 = 35;
    fake_temperature.val2 = 0;

    int ret = temp_alarm_check();

    zassert_equal(ret, 0,
                  "Temperature check should succeed");

    zassert_true(temp_alarm_is_active(),
                 "Alarm must activate above threshold");

    zassert_equal(fake_sensor_sample_fetch.call_count, 1,
                  "Sensor must be fetched once");

    zassert_equal(fake_sensor_channel_get.call_count, 1,
                  "Temperature must be read once");
}

/*
 * TASK 1: Reset clears the alarm
 */

ZTEST(temp_alarm, test_alarm_reset)
{
    fake_temperature.val1 = 40;

    zassert_ok(temp_alarm_check());

    zassert_true(temp_alarm_is_active(),
                 "Alarm should be active");

    temp_alarm_reset();

    zassert_false(temp_alarm_is_active(),
                  "Reset must clear alarm state");
}

/*
 * TASK 2: Fetch failure propagates
 */

ZTEST(temp_alarm, test_sensor_fetch_failure)
{
    fake_sensor_sample_fetch.return_val = -EIO;

    int ret = temp_alarm_check();

    zassert_equal(ret, -EIO,
                  "Fetch error must propagate");

    zassert_equal(fake_sensor_sample_fetch.call_count, 1,
                  "Fetch must be called");

    zassert_equal(fake_sensor_channel_get.call_count, 0,
                  "Channel get must not happen after fetch failure");

    zassert_false(temp_alarm_is_active(),
                  "Alarm should remain inactive");
}

/*
 * TASK 2: Channel read failure propagates
 */

ZTEST(temp_alarm, test_channel_get_failure)
{
    fake_sensor_channel_get.return_val = -EIO;

    int ret = temp_alarm_check();

    zassert_equal(ret, -EIO,
                  "Channel get error must propagate");

    zassert_equal(fake_sensor_sample_fetch.call_count, 1,
                  "Fetch must be called");

    zassert_equal(fake_sensor_channel_get.call_count, 1,
                  "Channel get must be called");
}

/*
 * TASK 2: Fail and then recover
 */

ZTEST(temp_alarm, test_fail_then_recover)
{
    /*
     * First operation fails.
     */

    fake_sensor_sample_fetch.return_val = -EIO;

    int ret = temp_alarm_check();

    zassert_equal(ret, -EIO,
                  "First operation must fail");

    zassert_false(temp_alarm_is_active(),
                  "Alarm must not activate after failure");

    /*
     * Simulate recovery.
     */

    fake_sensor_sample_fetch.return_val = 0;

    fake_temperature.val1 = 35;
    fake_temperature.val2 = 0;

    ret = temp_alarm_check();

    zassert_equal(ret, 0,
                  "Second operation should succeed");

    zassert_true(temp_alarm_is_active(),
                 "Alarm must activate after recovery");

    zassert_equal(fake_sensor_sample_fetch.call_count, 2,
                  "Fetch should have been called twice");

    zassert_equal(fake_sensor_channel_get.call_count, 1,
                  "Channel get should only occur on successful fetch");
}

/*
 * Test NULL sensor
 */

ZTEST(temp_alarm, test_null_sensor)
{
    temp_alarm_reset();

    int ret = temp_alarm_init(NULL);

    zassert_equal(ret, -EINVAL,
                  "NULL sensor must return -EINVAL");
}