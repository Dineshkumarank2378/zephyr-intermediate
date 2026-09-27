#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(race_demo, LOG_LEVEL_INF);

#define STACK_SIZE 1024

#define PRIO_A 3
#define PRIO_B 1

volatile int a = 0;

struct k_mutex  my_mutex ;




void thread_a_fn(void *p1, void *p2, void *p3)
{
    while (1) {
        if(k_mutex_lock(&my_mutex,K_FOREVER)==0)
        {
            int temp;

            temp = a;

            LOG_INF("THREAD A: read     = %d", temp);

            temp = temp + 1;

            a = temp;

            LOG_INF("THREAD A: wrote     = %d", a);

            k_mutex_unlock(&my_mutex);

            k_msleep(1000);
        }

        
    }
}

void thread_b_fn(void *p1, void *p2, void *p3)
{
    while (1) {

         if(k_mutex_lock(&my_mutex,K_FOREVER)==0)
        {

        int temp;

        temp = a;

        LOG_INF("THREAD B: read     = %d", temp);

        temp = temp + 1;

        a = temp;

        LOG_INF("THREAD B: wrote     = %d", a);

        k_msleep(20);

        k_mutex_unlock(&my_mutex);

        }
    }
}

K_THREAD_DEFINE(thread_a,
                STACK_SIZE,
                thread_a_fn,
                NULL, NULL, NULL,
                PRIO_A,
                0,
                0);

K_THREAD_DEFINE(thread_b,
                STACK_SIZE,
                thread_b_fn,
                NULL, NULL, NULL,
                PRIO_B,
                0,
                0);

int main(void)
{
     k_mutex_init (&my_mutex);
    return 0;
}