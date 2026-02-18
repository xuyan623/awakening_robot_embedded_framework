#include "core/data_struct/ringbuffer.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
typedef HANDLE host_thread_t;
typedef DWORD host_thread_ret_t;
#define HOST_THREAD_CALL WINAPI
static void host_thread_yield(void)
{
    (void)SwitchToThread();
}
#else
#include <pthread.h>
#include <sched.h>
typedef pthread_t host_thread_t;
typedef void* host_thread_ret_t;
#define HOST_THREAD_CALL
static void host_thread_yield(void)
{
    (void)sched_yield();
}
#endif

#define RINGBUFFER_CAPACITY      (4096U)
#define CONCURRENCY_TOTAL_ITEMS  (2000000U)

typedef struct RingbufferConcurrencyContext {
    Ringbuf_s rb;
    uint32_t totalItems;
    aw_atomic_uint_t produced;
    aw_atomic_uint_t consumed;
    aw_atomic_uint_t writeRetry;
    aw_atomic_uint_t readRetry;
    aw_atomic_uint_t errors;
    aw_atomic_uint_t stop;
} RingbufferConcurrencyContext_s;

static int run_ringbuffer_basic_test(void)
{
    Ringbuf_s rb;
    uint32_t storage[8] = {0};
    uint32_t inData[6] = {1, 2, 3, 4, 5, 6};
    uint32_t outData[6] = {0};
    uint32_t peekData[2] = {0};

    memset(&rb, 0, sizeof(rb));
    if (!ringbuf_init(&rb, (uint8_t*)storage, sizeof(uint32_t), 8))
        return 0;

    if (ringbuf_in(&rb, inData, 6) != 6)
        return 0;
    if (ringbuf_len(&rb) != 6)
        return 0;

    if (ringbuf_out(&rb, outData, 2) != 2)
        return 0;
    if (outData[0] != 1 || outData[1] != 2)
        return 0;

    if (ringbuf_out_peek(&rb, peekData, 2) != 2)
        return 0;
    if (peekData[0] != 3 || peekData[1] != 4)
        return 0;
    if (ringbuf_len(&rb) != 4)
        return 0;

    if (ringbuf_out(&rb, outData, 4) != 4)
        return 0;
    if (outData[0] != 3 || outData[1] != 4 || outData[2] != 5 || outData[3] != 6)
        return 0;
    if (!ringbuf_is_empty(&rb))
        return 0;

    return 1;
}

#ifdef _WIN32
static int host_thread_create(host_thread_t* thread, LPTHREAD_START_ROUTINE entry, void* arg)
{
    *thread = CreateThread(NULL, 0, entry, arg, 0, NULL);
    return (*thread == NULL) ? -1 : 0;
}

static int host_thread_join(host_thread_t thread)
{
    DWORD waitResult = WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return (waitResult == WAIT_OBJECT_0) ? 0 : -1;
}
#else
static int host_thread_create(host_thread_t* thread, void* (*entry)(void*), void* arg)
{
    return pthread_create(thread, NULL, entry, arg);
}

static int host_thread_join(host_thread_t thread)
{
    return pthread_join(thread, NULL);
}
#endif

static host_thread_ret_t HOST_THREAD_CALL producer_thread(void* arg)
{
    RingbufferConcurrencyContext_s* ctx = (RingbufferConcurrencyContext_s*)arg;
    uint32_t value = 1;
    while (value <= ctx->totalItems && aw_load_acq(&ctx->stop) == 0)
    {
        if (ringbuf_in(&ctx->rb, &value, 1) == 1)
        {
            aw_inc_ar(&ctx->produced);
            value++;
            continue;
        }

        aw_inc_ar(&ctx->writeRetry);
        host_thread_yield();
    }
#ifndef _WIN32
    return NULL;
#else
    return 0;
#endif
}

static host_thread_ret_t HOST_THREAD_CALL consumer_thread(void* arg)
{
    RingbufferConcurrencyContext_s* ctx = (RingbufferConcurrencyContext_s*)arg;
    uint32_t expected = 1;
    while (expected <= ctx->totalItems && aw_load_acq(&ctx->stop) == 0)
    {
        uint32_t value = 0;
        if (ringbuf_out(&ctx->rb, &value, 1) == 1)
        {
            if (value != expected)
            {
                aw_inc_ar(&ctx->errors);
                aw_store_rel(&ctx->stop, 1);
                break;
            }

            aw_inc_ar(&ctx->consumed);
            expected++;
            continue;
        }

        aw_inc_ar(&ctx->readRetry);
        host_thread_yield();
    }
#ifndef _WIN32
    return NULL;
#else
    return 0;
#endif
}

static int run_ringbuffer_concurrency_test(void)
{
    RingbufferConcurrencyContext_s ctx;
    host_thread_t producer;
    host_thread_t consumer;
    double elapsedSec;
    clock_t beginTicks;
    clock_t endTicks;

    memset(&ctx, 0, sizeof(ctx));
    ctx.totalItems = CONCURRENCY_TOTAL_ITEMS;
    aw_store_rlx(&ctx.produced, 0);
    aw_store_rlx(&ctx.consumed, 0);
    aw_store_rlx(&ctx.writeRetry, 0);
    aw_store_rlx(&ctx.readRetry, 0);
    aw_store_rlx(&ctx.errors, 0);
    aw_store_rlx(&ctx.stop, 0);

    if (!ringbuf_alloc(&ctx.rb, sizeof(uint32_t), RINGBUFFER_CAPACITY, NULL))
    {
        printf("[FAIL] ringbuf_alloc failed\n");
        return 0;
    }

    beginTicks = clock();

#ifdef _WIN32
    if (host_thread_create(&producer, producer_thread, &ctx) != 0 ||
        host_thread_create(&consumer, consumer_thread, &ctx) != 0)
#else
    if (host_thread_create(&producer, producer_thread, &ctx) != 0 ||
        host_thread_create(&consumer, consumer_thread, &ctx) != 0)
#endif
    {
        printf("[FAIL] thread create failed\n");
        free_ringbuf(&ctx.rb, NULL);
        return 0;
    }

    if (host_thread_join(producer) != 0 || host_thread_join(consumer) != 0)
    {
        printf("[FAIL] thread join failed\n");
        free_ringbuf(&ctx.rb, NULL);
        return 0;
    }

    endTicks = clock();
    elapsedSec = (double)(endTicks - beginTicks) / (double)CLOCKS_PER_SEC;

    printf("produced=%u consumed=%u write_retry=%u read_retry=%u errors=%u elapsed=%.3fs\n",
        aw_load_acq(&ctx.produced),
        aw_load_acq(&ctx.consumed),
        aw_load_acq(&ctx.writeRetry),
        aw_load_acq(&ctx.readRetry),
        aw_load_acq(&ctx.errors),
        elapsedSec);

    if (aw_load_acq(&ctx.errors) != 0 ||
        aw_load_acq(&ctx.produced) != ctx.totalItems ||
        aw_load_acq(&ctx.consumed) != ctx.totalItems ||
        !ringbuf_is_empty(&ctx.rb))
    {
        free_ringbuf(&ctx.rb, NULL);
        return 0;
    }

    free_ringbuf(&ctx.rb, NULL);
    return 1;
}

/**
 * 通过/失败标志
 * 通过：打印两条 [PASS]，且退出码 0
 * 失败：出现任一 [FAIL]，或 errors != 0，或 produced/consumed != totalItems，或 ringbuffer 非空，退出码非 0
 */

int main(void)
{
    int basicOk = run_ringbuffer_basic_test();
    int concurOk = run_ringbuffer_concurrency_test();

    if (!basicOk)
    {
        printf("[FAIL] ringbuffer basic test failed\n");
        return 1;
    }
    printf("[PASS] ringbuffer basic test passed\n");

    if (!concurOk)
    {
        printf("[FAIL] ringbuffer concurrency test failed\n");
        return 2;
    }
    printf("[PASS] ringbuffer concurrency test passed\n");

    return 0;
}

