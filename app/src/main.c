    #include <stdint.h>
#include <zephyr/kernel.h>
    #include <zephyr/logging/log.h>

    LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

    #define STACK_SIZE 1024

    #define PRIO_A 3
    #define PRIO_B 5
    #define PRIO_C 1
    #define PRIO_D -1
    #define PRIO_E -3



   
    void thread_a_fn(void *p1, void *p2, void *p3)
    {
        while (1) {
            LOG_INF("medium priority task is running\n");
            k_msleep(3000);
        }
    }

    void thread_b_fn(void *p1, void *p2, void *p3)
    {
        while (1) {
            LOG_INF("low priority task is running\n");
            LOG_INF("tick=%u", k_uptime_get_32());
            k_msleep(2100);
        }
    }

    void thread_c_fn(void *p1, void *p2, void *p3)
    {
        while (1) {
            LOG_INF("highest priority task is running\n");
            LOG_INF("tick=%u", k_uptime_get_32());
            k_msleep(2000);
        }
    } 
    
    void thread_d_fn(void *p1, void *p2, void *p3)
    {
        while (1) {
            for(int i=0;i<10;i++){ LOG_INF("%d  ",i);   for(volatile uint64_t j=0;j<1000000;j++);if(i==5) k_yield();}
            LOG_INF("\n");
            k_msleep(500);
        }
    }

     void thread_e_fn(void *p1, void *p2, void *p3)
    {
        while (1) {
            LOG_INF("hiiiiiiiiiiiiiiiiiiiiii\n");
            LOG_INF("tick=%u", k_uptime_get_32());
            k_msleep(1000);
        }
    }

    K_THREAD_DEFINE(thread_a, STACK_SIZE, thread_a_fn,
                    NULL, NULL, NULL, PRIO_A, 0, 0);
    K_THREAD_DEFINE(thread_b, STACK_SIZE, thread_b_fn,
                    NULL, NULL, NULL, PRIO_B, 0, 0);
    K_THREAD_DEFINE(thread_c, STACK_SIZE, thread_c_fn,NULL,NULL,NULL, PRIO_C,0,0);
    K_THREAD_DEFINE(thread_d, STACK_SIZE, thread_d_fn,NULL,NULL,NULL, PRIO_D,0,0);
    K_THREAD_DEFINE(thread_e, STACK_SIZE, thread_e_fn,NULL,NULL,NULL, PRIO_E,0,0);
    int main(void)
    {
        return 0;
    }

