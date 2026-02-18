#ifndef AW_BITMAP_H
#define AW_BITMAP_H

#include "atomic/aw_atomic.h"
#include "core/aw_def.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t bit_count;
    size_t word_count;
} AwBitmapMeta_s;

typedef struct {
    AwBitmapMeta_s meta;
    unsigned long *words;
} AwBitmapRaw_s;

typedef struct {
    AwBitmapMeta_s meta;
    aw_atomic_ulong_t *words;
} AwBitmapAtomic_s;

/**
 * @brief 计算位图需要的机器字数量
 */
size_t aw_bitmap_word_count(size_t bit_count);

/**
 * @brief 初始化 raw 位图（非线程安全）
 * @retval AWLF_OK 成功
 * @retval AWLF_ERROR_PARAM 参数非法（bitmap/buffer 为空或 bit_count 为 0）
 */
AwlfRet_e aw_bitmap_raw_init(AwBitmapRaw_s *bitmap, unsigned long *buffer, size_t bit_count);

/**
 * @brief 读取 raw 位
 */
bool aw_bitmap_raw_test(const AwBitmapRaw_s *bitmap, size_t bit_index);

/**
 * @brief 置位 raw 位
 */
void aw_bitmap_raw_set(AwBitmapRaw_s *bitmap, size_t bit_index);

/**
 * @brief 清位 raw 位
 */
void aw_bitmap_raw_clear(AwBitmapRaw_s *bitmap, size_t bit_index);

/**
 * @brief 初始化 atomic 位图（线程安全）
 * @retval AWLF_OK 成功
 * @retval AWLF_ERROR_PARAM 参数非法（bitmap/buffer 为空或 bit_count 为 0）
 */
AwlfRet_e aw_bitmap_atomic_init(AwBitmapAtomic_s *bitmap, aw_atomic_ulong_t *buffer, size_t bit_count);

/**
 * @brief 按 bit_count 计算并动态分配 raw 位图 buffer
 * @param bit_count 位数量
 * @param pmalloc 分配函数，可传 NULL（内部回退到 malloc）
 * @return 成功返回 buffer 指针，失败返回 NULL
 * @note 该接口仅负责 buffer 分配；需配合 aw_bitmap_raw_init 使用。
 */
unsigned long *aw_bitmap_raw_buffer_alloc(size_t bit_count, void *(*pmalloc)(size_t));

/**
 * @brief 按 bit_count 计算并动态分配 atomic 位图 buffer
 * @param bit_count 位数量
 * @param pmalloc 分配函数，可传 NULL（内部回退到 malloc）
 * @return 成功返回 buffer 指针，失败返回 NULL
 * @note 该接口仅负责 buffer 分配；需配合 aw_bitmap_atomic_init 使用。
 */
aw_atomic_ulong_t *aw_bitmap_atomic_buffer_alloc(size_t bit_count, void *(*pmalloc)(size_t));

/**
 * @brief 释放由 aw_bitmap_*_buffer_alloc 分配的 buffer
 * @param buffer buffer 指针
 * @param pfree 释放函数，可传 NULL（内部回退到 free）
 * @note 仅用于 alloc 路径分配的内存；外部静态/栈 buffer 禁止传入。
 */
void aw_bitmap_buffer_free(void *buffer, void (*pfree)(void *));

/**
 * @brief 读取 atomic 位（线程安全）
 */
bool aw_bitmap_atomic_test(const AwBitmapAtomic_s *bitmap, size_t bit_index);

/**
 * @brief 原子尝试置位
 * @retval true  从 0 -> 1 成功
 * @retval false 该位原本已为 1 或参数非法
 */
bool aw_bitmap_atomic_try_set(AwBitmapAtomic_s *bitmap, size_t bit_index);

/**
 * @brief 原子清位
 */
void aw_bitmap_atomic_clear(AwBitmapAtomic_s *bitmap, size_t bit_index);

/**
 * @brief 原子分配第一个空闲 bit（从 start_hint 开始循环扫描）
 * @retval AWLF_OK         分配成功
 * @retval AWLF_ERROR_BUSY 无可用 bit
 * @retval AWLF_ERROR_PARAM 参数非法
 */
AwlfRet_e aw_bitmap_atomic_alloc_first_zero(AwBitmapAtomic_s *bitmap, size_t start_hint, size_t *bit_out);

#ifdef __cplusplus
}
#endif

#endif // AW_BITMAP_H
