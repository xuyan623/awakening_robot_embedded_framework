#include "osal/osal.h"

/*
 * OSAL 综合测试示例：
 * 1) 线程 + 队列：生产者向队列投递数据，消费者取数据
 * 2) 事件：生产者向消费者发送事件通知
 * 3) 定时器 + 信号量：定时器周期触发，唤醒监控线程
 * 4) 互斥锁：保护共享计数器
 * 5) 时间接口：记录运行时间戳
 *
 * 使用方法：在 RTOS 工程中编译该文件，并在 main 中调用 osal_kernel_start()
 * 注意：本示例不依赖标准输出，建议通过断点或监视变量观察结果
 */

#define TEST_QUEUE_LEN 8U
#define TEST_TIMER_MS 100U
#define TEST_PROD_PERIOD 10U

typedef struct
{
    uint32_t seq;
    uint32_t time_ms;
} osal_test_msg_t;

static osal_queue_t g_queue;
static osal_mutex_t g_mutex;
static osal_sem_t g_sem;
static osal_event_flags_t g_event;
static osal_thread_t g_thread_prod;
static osal_thread_t g_thread_cons;
static osal_thread_t g_thread_mon;
static osal_thread_t g_thread_cnt;
static osal_thread_t g_thread_edge;
static osal_timer_t g_timer;

/* 运行状态统计，方便调试观察 */
static volatile uint32_t g_produced_cnt;
static volatile uint32_t g_consumed_cnt;
static volatile uint32_t g_monitor_cnt;
static volatile uint32_t g_shared_counter;
static volatile uint32_t g_counter_hits;
static volatile uint32_t g_edge_tests;
static volatile uint32_t g_edge_failures;
static volatile uint32_t g_edge_done;

static void _timer_cb(osal_timer_t timer)
{
    (void)timer;
    /* 定时器回调中发送信号量，唤醒监控线程 */
    (void)osal_sem_post(g_sem);
}

static void _producer_thread(void* arg)
{
    (void)arg;
    osal_test_msg_t msg;
    osal_time_ms_t last_ms = osal_time_now_monotonic();

    while (1)
    {
        /* 生成数据 */
        msg.seq = g_produced_cnt++;
        msg.time_ms = (uint32_t)osal_time_now_monotonic();

        /* 保护共享计数器 */
        if (osal_mutex_lock(g_mutex, OSAL_WAIT_FOREVER) == OSAL_OK)
        {
            g_shared_counter++;
            (void)osal_mutex_unlock(g_mutex);
        }

        /* 投递到队列，若超时则放弃本次 */
        (void)osal_queue_send(g_queue, &msg, 5U);

        /* 通知消费者线程（事件对象化，避免线程通知位冲突） */
        (void)osal_event_flags_set(g_event, 0x01U);

        /* 固定周期运行 */
        (void)osal_delay_until(&last_ms, TEST_PROD_PERIOD, NULL);
    }
}

static void _consumer_thread(void* arg)
{
    (void)arg;
    osal_test_msg_t msg;

    while (1)
    {
        uint32_t value = 0;
        /* 等待事件通知 */
        if (osal_event_flags_wait(g_event, 0x01U, &value, OSAL_WAIT_FOREVER, 0U) == OSAL_OK)
        {
            /* 收到事件后尝试取队列数据 */
            if (osal_queue_recv(g_queue, &msg, 10U) == OSAL_OK)
            {
                g_consumed_cnt++;
            }
        }
    }
}

static void _monitor_thread(void* arg)
{
    (void)arg;
    while (1)
    {
        /* 由定时器周期唤醒 */
        if (osal_sem_wait(g_sem, OSAL_WAIT_FOREVER) == OSAL_OK)
        {
            g_monitor_cnt++;
        }
    }
}

static void _counter_thread(void* arg)
{
    (void)arg;
    /* 高频竞争互斥锁，用于验证互斥效果 */
    while (1)
    {
        if (osal_mutex_lock(g_mutex, 1U) == OSAL_OK)
        {
            g_shared_counter++;
            g_counter_hits++;
            (void)osal_mutex_unlock(g_mutex);
        }
        /* 让出CPU，避免独占 */
        osal_sleep_ms(1U);
    }
}

static void _edge_thread(void* arg)
{
    (void)arg;
    uint32_t tests = 0;
    uint32_t failures = 0;

    /* 队列边界测试 */
    {
        osal_queue_t q = NULL;
        uint8_t item = 0x5A;

        tests++;
        if (osal_queue_create(NULL, 1U, 1U) != OSAL_INVALID)
            failures++;

        tests++;
        if (osal_queue_create(&q, 0U, 1U) != OSAL_INVALID)
            failures++;

        tests++;
        if (osal_queue_create(&q, 1U, 0U) != OSAL_INVALID)
            failures++;

        tests++;
        if (osal_queue_create(&q, 1U, 1U) != OSAL_OK)
            failures++;

        tests++;
        if (osal_queue_recv(q, &item, 0U) != OSAL_WOULD_BLOCK)
            failures++;

        tests++;
        if (osal_queue_send(q, NULL, 0U) != OSAL_INVALID)
            failures++;

        tests++;
        if (osal_queue_send(q, &item, 0U) != OSAL_OK)
            failures++;

        tests++;
        if (osal_queue_send(q, &item, 0U) != OSAL_WOULD_BLOCK)
            failures++;

        tests++;
        if (osal_queue_recv(q, &item, 0U) != OSAL_OK)
            failures++;

        tests++;
        if (osal_queue_delete(NULL) != OSAL_INVALID)
            failures++;

        tests++;
        if (osal_queue_delete(q) != OSAL_OK)
            failures++;
    }

    /* 信号量边界测试 */
    {
        osal_sem_t s = NULL;
        uint32_t sem_count = 0u;

        tests++;
        if (osal_sem_create(&s, 1U, 2U) != OSAL_INVALID)
            failures++;

        tests++;
        if (osal_sem_create(&s, 1U, 0U) != OSAL_OK)
            failures++;

        tests++;
        if (osal_sem_get_count(s, &sem_count) != OSAL_OK || sem_count != 0u)
            failures++;

        tests++;
        if (osal_sem_wait(s, 0U) != OSAL_WOULD_BLOCK)
            failures++;

        tests++;
        if (osal_sem_post(s) != OSAL_OK)
            failures++;

        tests++;
        if (osal_sem_get_count(s, &sem_count) != OSAL_OK || sem_count != 1u)
            failures++;

        tests++;
        if (osal_sem_post(s) != OSAL_NO_RESOURCE)
            failures++;

        tests++;
        if (osal_sem_wait(s, 0U) != OSAL_OK)
            failures++;

        tests++;
        if (osal_sem_get_count(s, &sem_count) != OSAL_OK || sem_count != 0u)
            failures++;

        tests++;
        if (osal_sem_delete(NULL) != OSAL_INVALID)
            failures++;

        tests++;
        if (osal_sem_delete(s) != OSAL_OK)
            failures++;
    }

    /* 互斥锁边界测试 */
    {
        osal_mutex_t m = NULL;

        tests++;
        if (osal_mutex_create(&m) != OSAL_OK)
            failures++;

        tests++;
        if (osal_mutex_unlock(m) != OSAL_INVALID)
            failures++;

        tests++;
        if (osal_mutex_lock(m, OSAL_WAIT_FOREVER) != OSAL_OK)
            failures++;

        tests++;
        if (osal_mutex_lock(m, 0U) != OSAL_WOULD_BLOCK)
            failures++;

        tests++;
        if (osal_mutex_unlock(m) != OSAL_OK)
            failures++;

        tests++;
        if (osal_mutex_delete(NULL) != OSAL_INVALID)
            failures++;

        tests++;
        if (osal_mutex_delete(m) != OSAL_OK)
            failures++;
    }

    /* 事件边界测试 */
    {
        uint32_t value = 0;
        osal_event_flags_t event = NULL;

        tests++;
        if (osal_event_flags_create(&event) != OSAL_OK)
            failures++;

        tests++;
        if (osal_event_flags_wait(event, 0x02U, &value, 0U, 0U) != OSAL_WOULD_BLOCK)
            failures++;

        (void)osal_event_flags_delete(event);
    }

    /* 定时器边界测试 */
    {
        tests++;
        if (osal_timer_create("bad_timer", 0U, 1, NULL, _timer_cb) != NULL)
            failures++;

        tests++;
        if (osal_timer_create("bad_timer2", 1U, 1, NULL, NULL) != NULL)
            failures++;
    }

    /* 时间接口边界测试 */
    {
        osal_time_ms_t ms = osal_time_now_monotonic();
        osal_time_ms_t ms_next = osal_time_now_monotonic();
        tests++;
        if (osal_time_before(ms_next, ms))
            failures++;
    }

    g_edge_tests = tests;
    g_edge_failures = failures;
    g_edge_done = 1U;

    while (1)
    {
        osal_sleep_ms(1000U);
    }
}

int main(void)
{
    /* 创建事件对象 */
    if (osal_event_flags_create(&g_event) != OSAL_OK)
        return -1;

    /* 创建队列 */
    if (osal_queue_create(&g_queue, TEST_QUEUE_LEN, sizeof(osal_test_msg_t)) != OSAL_OK)
        return -1;

    /* 创建互斥锁 */
    if (osal_mutex_create(&g_mutex) != OSAL_OK)
        return -1;

    /* 创建信号量 */
    if (osal_sem_create(&g_sem, 10U, 0U) != OSAL_OK)
        return -1;

    /* 创建线程 */
    osal_thread_attr_t attr_prod = {"osal_prod", 512U * OSAL_STACK_WORD_BYTES, 2U};
    osal_thread_attr_t attr_cons = {"osal_cons", 512U * OSAL_STACK_WORD_BYTES, 2U};
    osal_thread_attr_t attr_mon = {"osal_mon", 512U * OSAL_STACK_WORD_BYTES, 1U};
    osal_thread_attr_t attr_cnt = {"osal_cnt", 512U * OSAL_STACK_WORD_BYTES, 1U};
    osal_thread_attr_t attr_edge = {"osal_edge", 768U * OSAL_STACK_WORD_BYTES, 2U};

    if (osal_thread_create(&g_thread_prod, &attr_prod, _producer_thread, NULL) != OSAL_OK)
        return -1;
    if (osal_thread_create(&g_thread_cons, &attr_cons, _consumer_thread, NULL) != OSAL_OK)
        return -1;
    if (osal_thread_create(&g_thread_mon, &attr_mon, _monitor_thread, NULL) != OSAL_OK)
        return -1;
    if (osal_thread_create(&g_thread_cnt, &attr_cnt, _counter_thread, NULL) != OSAL_OK)
        return -1;
    if (osal_thread_create(&g_thread_edge, &attr_edge, _edge_thread, NULL) != OSAL_OK)
        return -1;

    /* 创建并启动定时器 */
    g_timer = osal_timer_create("osal_timer", TEST_TIMER_MS, 1, NULL, _timer_cb);
    if (!g_timer)
        return -1;
    if (osal_timer_start(g_timer, OSAL_WAIT_FOREVER) != OSAL_OK)
        return -1;

    /* 启动调度器 */
    return osal_kernel_start();
}
