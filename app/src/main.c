#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/task_wdt/task_wdt.h>

#include <stdint.h>

LOG_MODULE_REGISTER(l5_homework, LOG_LEVEL_INF);

/* ---------------- Configuration ---------------- */

#define QUEUE_DEPTH          8
#define PRODUCER_PERIOD_MS   250
#define CONSUMER_PERIOD_MS   600

#define CONSUMER_WDT_TIMEOUT_MS  2000

#define HEALTH_CHECK_PERIOD_MS   1000
#define QUEUE_WARNING_LEVEL      75

#define SIMULATE_STUCK_CONSUMER 1
#define STUCK_AFTER_SEQUENCE    5
#define STUCK_DURATION_SECONDS  5

/* ---------------- Data Structure ---------------- */

struct sensor_data {
    uint32_t sequence;
    int32_t value;
};

/* ---------------- Message Queue ---------------- */

K_MSGQ_DEFINE(sensor_queue,
              sizeof(struct sensor_data),
              QUEUE_DEPTH,
              4);

/* ---------------- Thread Configuration ---------------- */

#define THREAD_STACK_SIZE 1024

#define PRODUCER_PRIORITY 5
#define CONSUMER_PRIORITY 5
#define HEALTH_PRIORITY   6

K_THREAD_STACK_DEFINE(producer_stack, THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(consumer_stack, THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(health_stack, THREAD_STACK_SIZE);

static struct k_thread producer_thread;
static struct k_thread consumer_thread;
static struct k_thread health_thread;

/* ---------------- Watchdog ---------------- */

static int consumer_wdt_channel;

/*
 * Called when the consumer fails to feed its watchdog
 * within the configured timeout.
 */
static void consumer_wdt_callback(int channel, void *user_data)
{
    ARG_UNUSED(user_data);

    LOG_ERR("WATCHDOG TIMEOUT! Consumer thread is not responding!");

    LOG_ERR("Watchdog channel: %d", channel);
}

/* ---------------- Producer Thread ---------------- */

static void producer_fn(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    uint32_t sequence = 0;

    while (1) {

        struct sensor_data data;

        data.sequence = sequence;
        data.value = 100 + sequence;

        /*
         * K_NO_WAIT means:
         * Do not block if the queue is full.
         */
        int ret = k_msgq_put(&sensor_queue,
                             &data,
                             K_NO_WAIT);

        if (ret == 0) {

            LOG_INF("PRODUCER: Sent sequence=%u value=%d",
                    data.sequence,
                    data.value);
        }
        else {

            LOG_WRN("PRODUCER: Queue FULL! Dropping sequence=%u",
                    sequence);
        }

        sequence++;

        k_msleep(PRODUCER_PERIOD_MS);
    }
}

/* ---------------- Consumer Thread ---------------- */

static void consumer_fn(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    struct sensor_data data;

    bool stuck_simulation_done = false;

    while (1) {

        /*
         * Wait until data is available.
         */
        int ret = k_msgq_get(&sensor_queue,
                             &data,
                             K_FOREVER);

        if (ret == 0) {

            LOG_INF("CONSUMER: Received sequence=%u value=%d",
                    data.sequence,
                    data.value);

#if SIMULATE_STUCK_CONSUMER

            /*
             * Simulate a consumer becoming stuck.
             *
             * The consumer will sleep for 5 seconds.
             * Its watchdog timeout is only 2 seconds.
             */
            if ((data.sequence >= STUCK_AFTER_SEQUENCE) &&
                (!stuck_simulation_done)) {

                stuck_simulation_done = true;

                LOG_ERR("CONSUMER: SIMULATING STUCK CONDITION!");

                LOG_ERR("CONSUMER: Sleeping for %d seconds",
                        STUCK_DURATION_SECONDS);

                k_sleep(K_SECONDS(STUCK_DURATION_SECONDS));

                LOG_WRN("CONSUMER: Recovered from simulated stuck condition");
            }

#endif

            /*
             * Normal processing.
             */
            LOG_INF("CONSUMER: Processing sequence=%u",
                    data.sequence);

            k_msleep(CONSUMER_PERIOD_MS);

            /*
             * Feed watchdog only after successful processing.
             *
             * If the consumer gets stuck above, it cannot
             * reach this statement before the timeout.
             */
            ret = task_wdt_feed(consumer_wdt_channel);

            if (ret == 0) {

                LOG_DBG("CONSUMER: Watchdog fed successfully");
            }
            else {

                LOG_WRN("CONSUMER: Watchdog feed failed: %d",
                        ret);
            }
        }
    }
}

/* ---------------- Health Check Thread ---------------- */

static void health_check_fn(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    while (1) {

        /*
         * Get the number of messages currently in the queue.
         */
        uint32_t used = k_msgq_num_used_get(&sensor_queue);

        /*
         * Get the maximum queue capacity.
         */
        uint32_t free_space = k_msgq_num_free_get(&sensor_queue);

        uint32_t percentage =
            (used * 100) / QUEUE_DEPTH;

        LOG_INF("HEALTH: Queue usage=%u/%u (%u%%), free=%u",
                used,
                QUEUE_DEPTH,
                percentage,
                free_space);

        /*
         * Warn when queue usage reaches 75%.
         */
        if (percentage >= QUEUE_WARNING_LEVEL) {

            LOG_WRN("HEALTH WARNING: Queue usage is above %d%%!",
                    QUEUE_WARNING_LEVEL);

            LOG_WRN("HEALTH WARNING: Consumer may be slow or stuck");
        }

        k_msleep(HEALTH_CHECK_PERIOD_MS);
    }
}

/* ---------------- Main ---------------- */

int main(void)
{
    LOG_INF("Lecture 5 Homework");
    LOG_INF("Memory, Resource Constraints & Reliability");

    /* ---------------- Initialize Task Watchdog ---------------- */

    /*
     * NULL means software-only task watchdog.
     */
    int ret = task_wdt_init(NULL);

    if (ret != 0) {

        LOG_ERR("Task watchdog initialization failed: %d",
                ret);

        return ret;
    }

    LOG_INF("Task watchdog initialized");

    /*
     * Register consumer with a 2-second timeout.
     */
    consumer_wdt_channel =
        task_wdt_add(CONSUMER_WDT_TIMEOUT_MS,
                     consumer_wdt_callback,
                     NULL);

    if (consumer_wdt_channel < 0) {

        LOG_ERR("Failed to register consumer watchdog");

        return consumer_wdt_channel;
    }

    LOG_INF("Consumer watchdog registered");
    LOG_INF("Watchdog timeout: %d ms",
            CONSUMER_WDT_TIMEOUT_MS);

    /* ---------------- Create Producer ---------------- */

    k_thread_create(&producer_thread,
                    producer_stack,
                    K_THREAD_STACK_SIZEOF(producer_stack),
                    producer_fn,
                    NULL,
                    NULL,
                    NULL,
                    PRODUCER_PRIORITY,
                    0,
                    K_NO_WAIT);

    k_thread_name_set(&producer_thread,
                      "producer");

    /* ---------------- Create Consumer ---------------- */

    k_thread_create(&consumer_thread,
                    consumer_stack,
                    K_THREAD_STACK_SIZEOF(consumer_stack),
                    consumer_fn,
                    NULL,
                    NULL,
                    NULL,
                    CONSUMER_PRIORITY,
                    0,
                    K_NO_WAIT);

    k_thread_name_set(&consumer_thread,
                      "consumer");

    /* ---------------- Create Health Check ---------------- */

    k_thread_create(&health_thread,
                    health_stack,
                    K_THREAD_STACK_SIZEOF(health_stack),
                    health_check_fn,
                    NULL,
                    NULL,
                    NULL,
                    HEALTH_PRIORITY,
                    0,
                    K_NO_WAIT);

    k_thread_name_set(&health_thread,
                      "health_check");

    LOG_INF("All threads started");

    return 0;
}