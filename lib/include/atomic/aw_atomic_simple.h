#ifndef AW_ATOMIC_SIMPLE_H
#define AW_ATOMIC_SIMPLE_H

#include "aw_atomic.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * ============================================================================
 * AW Atomic Simplified Library (Rel/Acq Model)
 * ============================================================================
 * * 命名约定:
 * - _rlx : Relaxed (松散模型，仅保证原子性，无同步语义)
 * - _acq : Acquire (获取语义，用于 Load 或 RMW)
 * - _rel : Release (释放语义，用于 Store 或 RMW)
 * - _ar  : AcqRel  (获取+释放，用于 RMW)
 * * 所有的指针参数 (ptr) 均通过底层 aw_atomic.h 处理，
 * 因此本库不关心具体数据类型 (int, long, void* 等)。
 */

// ============================================================================
// 1. Load (读取)
// ============================================================================

// 最常用的同步读取，保证后续读写不越过此点
#define aw_load_acq(ptr) aw_load(ptr, AW_MO_ACQUIRE)

// 仅保证原子读取，无顺序保证 (常用于统计计数读取)
#define aw_load_rlx(ptr) aw_load(ptr, AW_MO_RELAXED)

// 消费语义 (在某些架构上比 Acquire 轻量，但通常等同于 Acquire)
#define aw_load_consume(ptr) aw_load(ptr, AW_MO_CONSUME)

// ============================================================================
// 2. Store (写入)
// ============================================================================

// 最常用的同步写入，保证之前的读写已完成 (常用于发布数据)
#define aw_store_rel(ptr, val) aw_store(ptr, val, AW_MO_RELEASE)

// 仅保证原子写入 (常用于重置计数器)
#define aw_store_rlx(ptr, val) aw_store(ptr, val, AW_MO_RELAXED)

// ============================================================================
// 3. Exchange (交换 / Swap)
// ============================================================================

// 强同步交换 (常用于锁的获取与释放)
#define aw_swap_ar(ptr, val) aw_exchange(ptr, val, AW_MO_ACQ_REL)

// 获取语义交换 (常用于获取锁)
#define aw_swap_acq(ptr, val) aw_exchange(ptr, val, AW_MO_ACQUIRE)

// 释放语义交换 (常用于释放锁并写入新值)
#define aw_swap_rel(ptr, val) aw_exchange(ptr, val, AW_MO_RELEASE)

// 松散交换
#define aw_swap_rlx(ptr, val) aw_exchange(ptr, val, AW_MO_RELAXED)

// ============================================================================
// 4. CAS (Compare And Swap)
// ============================================================================
// 注意: aw_cas 原型为 (ptr, expected_ptr, desired, succ_order, fail_order)
// 本库中 CAS 失败时的内存序默认设置为 Acquire (如果成功序包含Acquire) 或 Relaxed

// [最常用] 强 CAS，成功时全屏障，失败时获取最新值 (适用于大多数无锁结构)
#define aw_cas_ar(ptr, exp_ptr, des) aw_cas(ptr, exp_ptr, des, AW_MO_ACQ_REL, AW_MO_ACQUIRE)

// 获取型 CAS (成功和失败都具备 Acquire 语义)
#define aw_cas_acq(ptr, exp_ptr, des) aw_cas(ptr, exp_ptr, des, AW_MO_ACQUIRE, AW_MO_ACQUIRE)

// 释放型 CAS (成功时 Release，失败时 Relaxed)
#define aw_cas_rel(ptr, exp_ptr, des) aw_cas(ptr, exp_ptr, des, AW_MO_RELEASE, AW_MO_RELAXED)

// 松散 CAS (无同步语义)
#define aw_cas_rlx(ptr, exp_ptr, des) aw_cas(ptr, exp_ptr, des, AW_MO_RELAXED, AW_MO_RELAXED)

// ============================================================================
// 5. Arithmetic (Fetch Add / Sub)
// ============================================================================

// --- Fetch Add (返回旧值) ---
#define aw_faa_rlx(ptr, val) aw_fetch_add(ptr, val, AW_MO_RELAXED)
#define aw_faa_acq(ptr, val) aw_fetch_add(ptr, val, AW_MO_ACQUIRE)
#define aw_faa_rel(ptr, val) aw_fetch_add(ptr, val, AW_MO_RELEASE)
#define aw_faa_ar(ptr, val) aw_fetch_add(ptr, val, AW_MO_ACQ_REL)

// --- Fetch Sub (返回旧值) ---
#define aw_fas_rlx(ptr, val) aw_fetch_sub(ptr, val, AW_MO_RELAXED)
#define aw_fas_acq(ptr, val) aw_fetch_sub(ptr, val, AW_MO_ACQUIRE)
#define aw_fas_rel(ptr, val) aw_fetch_sub(ptr, val, AW_MO_RELEASE)
#define aw_fas_ar(ptr, val) aw_fetch_sub(ptr, val, AW_MO_ACQ_REL)

// --- Helper: Increment / Decrement (常用操作) ---
#define aw_inc_rlx(ptr) aw_faa_rlx(ptr, 1)
#define aw_inc_ar(ptr) aw_faa_ar(ptr, 1)
#define aw_dec_rlx(ptr) aw_fas_rlx(ptr, 1)
#define aw_dec_ar(ptr) aw_fas_ar(ptr, 1)

// ============================================================================
// 6. Bitwise Logic (Fetch And / Or / Xor)
// ============================================================================

// --- Fetch And (按位与) ---
#define aw_fand_rlx(ptr, val) aw_fetch_and(ptr, val, AW_MO_RELAXED)
#define aw_fand_acq(ptr, val) aw_fetch_and(ptr, val, AW_MO_ACQUIRE)
#define aw_fand_rel(ptr, val) aw_fetch_and(ptr, val, AW_MO_RELEASE)
#define aw_fand_ar(ptr, val) aw_fetch_and(ptr, val, AW_MO_ACQ_REL)

// --- Fetch Or (按位或 - 常用于设置标志位) ---
#define aw_for_rlx(ptr, val) aw_fetch_or(ptr, val, AW_MO_RELAXED)
#define aw_for_acq(ptr, val) aw_fetch_or(ptr, val, AW_MO_ACQUIRE)
#define aw_for_rel(ptr, val) aw_fetch_or(ptr, val, AW_MO_RELEASE)
#define aw_for_ar(ptr, val) aw_fetch_or(ptr, val, AW_MO_ACQ_REL)

// --- Fetch Xor (按位异或 - 常用于翻转位) ---
#define aw_fxor_rlx(ptr, val) aw_fetch_xor(ptr, val, AW_MO_RELAXED)
#define aw_fxor_acq(ptr, val) aw_fetch_xor(ptr, val, AW_MO_ACQUIRE)
#define aw_fxor_rel(ptr, val) aw_fetch_xor(ptr, val, AW_MO_RELEASE)
#define aw_fxor_ar(ptr, val) aw_fetch_xor(ptr, val, AW_MO_ACQ_REL)

// ============================================================================
// 7. Fences (内存屏障)
// ============================================================================

// 线程屏障 (全局可见性同步)
#define aw_fence_acq() aw_thread_fence(AW_MO_ACQUIRE)
#define aw_fence_rel() aw_thread_fence(AW_MO_RELEASE)
#define aw_fence_ar() aw_thread_fence(AW_MO_ACQ_REL)
#define aw_fence_seq() aw_thread_fence(AW_MO_SEQ_CST) // 最强屏障

// 编译器/信号屏障 (防止编译器重排，不生成 CPU 指令)
#define aw_compiler_barrier() aw_signal_fence(AW_MO_ACQ_REL)

#ifdef __cplusplus
}
#endif

#endif // AW_ATOMIC_SIMPLE_H