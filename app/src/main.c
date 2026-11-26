#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(l4_demo, LOG_LEVEL_INF);

#define STACK_SIZE 2048
#define SENSOR_PERIOD_MS 100
#define SENSOR_COUNT 10

// Sensor data structure
struct sensor_data {
    int32_t temperature_mc;
    uint8_t seq;
};

// Forward decl
static void display_listener_cb(const struct zbus_channel *chan);

// Listener setup
ZBUS_LISTENER_DEFINE(display_listener, display_listener_cb);

// Subscriber setup - buffer size 4
ZBUS_SUBSCRIBER_DEFINE(logger_subscriber, 4);

// Channel definition with initial values
ZBUS_CHAN_DEFINE(sensor_channel,
                 struct sensor_data,
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS(display_listener, logger_subscriber),
                 ZBUS_MSG_INIT(.temperature_mc = 0,
                               .seq = 0));

/* ========================================================= */
/* Display Listener                                          */
/* ========================================================= */

static void display_listener_cb(const struct zbus_channel *chan)
{
    const struct sensor_data *msg = zbus_chan_const_msg(chan);

    LOG_INF("[DISPLAY] seq=%u temp=%d mC",
            msg->seq,
            msg->temperature_mc);
}

/* ========================================================= */
/* Sensor Thread - Publisher                                 */
/* ========================================================= */

static void sensor_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    for (int n = 0; n < SENSOR_COUNT; n++) {

        struct sensor_data reading = {
            .temperature_mc = 25000 + (n * 1000),
            .seq = n,
        };

        LOG_INF("[SENSOR] Publishing seq=%u temp=%d mC",
                reading.seq,
                reading.temperature_mc);

        int status = zbus_chan_pub(&sensor_channel,
                                &reading,
                                K_MSEC(100));

        if (status != 0) {
            LOG_ERR("Pub failed: %d", status);
        }

        // Small delay between publishes
        k_msleep(SENSOR_PERIOD_MS);
    }

    LOG_INF("[SENSOR] Done publishing");
}

/* ========================================================= */
/* Logger Thread - Subscriber                                */
/* ========================================================= */

static void logger_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    const struct zbus_channel *chan;

    while (1) {
        // Wait for notification
        int status = zbus_sub_wait(&logger_subscriber,
                                &chan,
                                K_FOREVER);

        if (status != 0) {
            LOG_ERR("Wait failed: %d", status);
            continue;
        }

        struct sensor_data data;

        // Read latest message
        status = zbus_chan_read(chan, &data, K_MSEC(100));

        if (status != 0) {
            LOG_ERR("Read failed: %d", status);
            continue;
        }

        LOG_INF("[LOGGER] Got seq=%u temp=%d mC",
                data.seq,
                data.temperature_mc);

        // Simulate slow logging I/O
        k_msleep(250);
    }
}

/* ========================================================= */
/* Threads                                                    */
/* ========================================================= */

K_THREAD_DEFINE(sensor_thread,
               STACK_SIZE,
               sensor_thread_fn,
               NULL, NULL, NULL,
               5,
               0,
               0);

K_THREAD_DEFINE(logger_thread,
               STACK_SIZE,
               logger_thread_fn,
               NULL, NULL, NULL,
               6,
               0,
               0);

/* ========================================================= */
/* Main                                                       */
/* ========================================================= */

int main(void)
{
    LOG_INF("=================================");
    LOG_INF("Lecture 4 - Zbus Pub/Sub Demo");
    LOG_INF("=================================");

    LOG_INF("Sensor publishes every %d ms", SENSOR_PERIOD_MS);
    LOG_INF("Display uses a Listener");
    LOG_INF("Logger uses a Subscriber");

    return 0;
}