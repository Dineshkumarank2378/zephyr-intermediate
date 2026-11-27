#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(l6_homework, LOG_LEVEL_INF);

#define STACK_SIZE 1024

#define PRODUCER_PRIORITY 5
#define CONSUMER_PRIORITY 4

#define PRODUCER_PERIOD_MS 250
#define DEADLINE_MS 100
#define PROCESSING_DELAY_MS 20

#define MESSAGE_QUEUE_SIZE 10

struct sensor_msg {
    uint32_t sequence;
    int64_t timestamp;
};

K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_msg), MESSAGE_QUEUE_SIZE, 4);

static atomic_t produced_count;
static atomic_t completed_count;
static atomic_t deadline_miss_count;

static uint32_t total_response_time_ms;
static uint32_t max_response_time_ms;

K_THREAD_STACK_DEFINE(producer_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(consumer_stack, STACK_SIZE);

static struct k_thread producer_thread;
static struct k_thread consumer_thread;

static void producer_thread_fn(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    uint32_t sequence = 0;

    LOG_INF("Producer started");

    while (1) {
        struct sensor_msg msg;

        msg.sequence = sequence++;
        msg.timestamp = k_uptime_get();

        int ret = k_msgq_put(&sensor_msgq, &msg, K_NO_WAIT);

        if (ret == 0) {
            atomic_inc(&produced_count);

            LOG_INF("event_ready seq=%u queue=%u",
                    msg.sequence,
                    k_msgq_num_used_get(&sensor_msgq));
        } else {
            LOG_WRN("Queue full, dropping seq=%u", msg.sequence);
        }

        k_sleep(K_MSEC(PRODUCER_PERIOD_MS));
    }
}

static void consumer_thread_fn(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    LOG_INF("Consumer started");

    while (1) {
        struct sensor_msg msg;

        int ret = k_msgq_get(&sensor_msgq, &msg, K_FOREVER);

        if (ret != 0) {
            continue;
        }

        k_sleep(K_MSEC(PROCESSING_DELAY_MS));

        int64_t current_time = k_uptime_get();

        uint32_t response_time_ms =
            (uint32_t)(current_time - msg.timestamp);

        atomic_inc(&completed_count);

        total_response_time_ms += response_time_ms;

        if (response_time_ms > max_response_time_ms) {
            max_response_time_ms = response_time_ms;
        }

        if (response_time_ms > DEADLINE_MS) {
            atomic_inc(&deadline_miss_count);

            LOG_WRN("deadline_miss count=%d seq=%u latency_ms=%u",
                    atomic_get(&deadline_miss_count),
                    msg.sequence,
                    response_time_ms);
        } else {
            LOG_INF("event_done seq=%u latency_ms=%u",
                    msg.sequence,
                    response_time_ms);
        }

        if ((atomic_get(&completed_count) % 5) == 0) {
            uint32_t average_response_time =
                total_response_time_ms /
                (uint32_t)atomic_get(&completed_count);

            LOG_INF("==============================");

            LOG_INF("Produced: %d",
                    atomic_get(&produced_count));

            LOG_INF("Completed: %d",
                    atomic_get(&completed_count));

            LOG_INF("Deadline misses: %d",
                    atomic_get(&deadline_miss_count));

            LOG_INF("Average latency: %u ms",
                    average_response_time);

            LOG_INF("Maximum latency: %u ms",
                    max_response_time_ms);

            LOG_INF("Queue usage: %u",
                    k_msgq_num_used_get(&sensor_msgq));

            LOG_INF("==============================");
        }
    }
}

int main(void)
{
    LOG_INF("Lecture 6 Runtime Analysis");

    LOG_INF("Deadline: %d ms", DEADLINE_MS);
    LOG_INF("Processing delay: %d ms", PROCESSING_DELAY_MS);

    k_thread_create(&producer_thread,
                    producer_stack,
                    K_THREAD_STACK_SIZEOF(producer_stack),
                    producer_thread_fn,
                    NULL,
                    NULL,
                    NULL,
                    PRODUCER_PRIORITY,
                    0,
                    K_NO_WAIT);

    k_thread_name_set(&producer_thread, "producer");

    k_thread_create(&consumer_thread,
                    consumer_stack,
                    K_THREAD_STACK_SIZEOF(consumer_stack),
                    consumer_thread_fn,
                    NULL,
                    NULL,
                    NULL,
                    CONSUMER_PRIORITY,
                    0,
                    K_NO_WAIT);

    k_thread_name_set(&consumer_thread, "consumer");

    LOG_INF("Threads created");

    return 0;
}