/*
 * Lecture 3 - Homework
 *
 * GOAL: Demonstrate event-driven execution using Zephyr workqueues.
 *
 * BONUS:
 * Generate 50 events at 5ms intervals.
 * Use k_work_reschedule() with a 30ms delay.
 * Only one handler execution should occur after the event burst.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(homework, LOG_LEVEL_DBG);

#define STACK_SIZE    1024
#define EVENT_DELAY   5
#define DEBOUNCE_MS   30
#define EVENT_COUNT   5

/* Statistics */
static int total_events;
static int total_processed;

/* Work handler declaration */
static void handler_fn(struct k_work *work);

/* Define delayable work item */
K_WORK_DELAYABLE_DEFINE(work, handler_fn);

/* ---------------------------------------------------------- */
/* Sensor simulation thread                                   */
/* ---------------------------------------------------------- */

static void sensor_sim_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    for (int i = 0; i < EVENT_COUNT; i++) {

        k_msleep(EVENT_DELAY);

        total_events++;

        LOG_INF("[SENSOR] event %d tick=%u",
                i, k_uptime_get_32());

        int ret = k_work_reschedule(&work, K_MSEC(DEBOUNCE_MS));

        if (ret < 0) {
            LOG_ERR("Work reschedule failed: %d", ret);
        }
    }

    LOG_INF("[SENSOR] all events produced");
}

/* ---------------------------------------------------------- */
/* Work handler                                               */
/* ---------------------------------------------------------- */

static void handler_fn(struct k_work *work)
{
    ARG_UNUSED(work);

    total_processed++;

    LOG_INF("[CONSUMER] processed event %d tick=%u",
            total_processed, k_uptime_get_32());
}

/* ---------------------------------------------------------- */
/* Thread definition                                          */
/* ---------------------------------------------------------- */

K_THREAD_DEFINE(sensor_thread,
                STACK_SIZE,
                sensor_sim_fn,
                NULL, NULL, NULL,
                5, 0, 0);

/* ---------------------------------------------------------- */
/* Main                                                       */
/* ---------------------------------------------------------- */

int main(void)
{
    LOG_INF("=== L3 Homework: Workqueue Debouncing ===");
    LOG_INF("Sensor event interval: %d ms", EVENT_DELAY);
    LOG_INF("Debounce delay: %d ms", DEBOUNCE_MS);
    LOG_INF("Total events: %d", EVENT_COUNT);

    /* Wait for sensor events and final handler execution */
    k_msleep((EVENT_COUNT * EVENT_DELAY) + DEBOUNCE_MS + 100);

    LOG_INF("[SUMMARY] Total events: %d", total_events);
    LOG_INF("[SUMMARY] Total processed: %d", total_processed);

    return 0;
}