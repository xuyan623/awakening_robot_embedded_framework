# AW Atomics: 一个轻量级、Header-Only、跨平台 C 语言原子操作库

## 1. 架构与设计思想

`AW Atomics` 旨在为 C 语言开发者提供一套统一、高性能且易于使用的原子操作接口，消除不同编译器和标准版本之间的差异。

### 1.1 分层架构

该库采用清晰的分层设计，以确保极高的可维护性和扩展性：

- **基础适配层 (`aw_atomic_base.h`)**：负责编译器检测（MSVC, GCC, Clang, AC5/AC6）和标准环境探测（C11 `stdatomic.h`），定义了统一的内存序枚举 `aw_memory_order` 和原子变量修饰宏 `aw_atomic_t`。
- **后端实现层 (`aw_atomic_gcc.h`, `aw_atomic_msvc.h`, `aw_atomic_ac5.h`)**：针对不同编译器调用对应的内置函数或内联汇编。
- **核心接口层 (`aw_atomic.h`)**：提供统一的函数式宏 API，如 `aw_load` 和 `aw_cas`。它利用 `_Generic`或复杂的宏逻辑实现泛型支持，自动识别 8/16/32/64 位及指针类型。
- **简化应用层 (`aw_atomic_simple.h`)**：针对最常用的内存模型（Acquire/Release）封装了更短的 API（如 `aw_load_acq`），降低使用门槛。

### 1.2 核心设计理念

- **Header-only (纯头文件)**：无需编译，只需将头文件加入项目路径即可使用。
- **C11 优先与回退机制**：在支持 C11 以上的环境下优先调用标准 `stdatomic.h`，在旧编译器或嵌入式环境（如 ARMCC AC5）下自动回退到编译器特有的原子实现。
- **类型安全**：通过 `_Generic` 选择匹配宽度的后端函数，确保在 MSVC 等非泛型环境中依然保持类型转换的正确性。
- **高性能自旋优化**：内置 `aw_cpu_pause()`，针对不同架构（x86 的 `pause`，ARM 的 `yield`）提供自旋锁优化指令。

------

## 2. API手册

### 2.1 类型系统与宏

该库定义了一套跨平台的原子类型别名，确保在不同编译器下均能实现原子性访问。

| **类型别名**         | **原始类型映射**     | **备注**     |
| -------------------- | -------------------- | ------------ |
| `aw_atomic_int_t`    | `int`                | 标准原子整型 |
| `aw_atomic_uint_t`   | `unsigned int`       |              |
| `aw_atomic_long_t`   | `long`               |              |
| `aw_atomic_ulong_t`  | `unsigned long`      |              |
| `aw_atomic_llong_t`  | `long long`          |              |
| `aw_atomic_ullong_t` | `unsigned long long` |              |
| `aw_atomic_ptr_t`    | `void*`              | 原子指针     |
| `aw_atomic_size_t`   | `size_t`             |              |

#### 变量管理宏

- **`aw_atomic_t(type)`**: 通用类型包装宏。在 C语言标准库 下展开为 `_Atomic(type)`，在旧版本下展开为 `volatile type`。
- **`AW_ATOMIC_VAR_INIT(val)`**: 用于初始化原子变量。注：在 C17 后建议直接赋值，该宏为保持向前兼容而存在。
- **`aw_cpu_pause()`**: CPU 自旋优化指令。在 x86 上执行 `pause`，在 ARM 上执行 `yield`，在 MSVC 下调用 `_mm_pause()`。

------

### 2.1 核心原子操作 (`aw_atomic.h`)

这是库的核心接口，所有宏均支持**泛型参数**。

#### 2.1.1 内存序枚举 (`aw_memory_order`)

- `AW_MO_RELAXED`: 无同步语义。
- `AW_MO_CONSUME`: 消费语义。
- `AW_MO_ACQUIRE`: 获取语义。
- `AW_MO_RELEASE`: 释放语义。
- `AW_MO_ACQ_REL`: 获取+释放语义。
- `AW_MO_SEQ_CST`: 顺序一致性（全屏障）。

#### 2.1.2 基础读写

- **`aw_load(ptr, order)`**: 从 `ptr` 原子地读取值。
- **`aw_store(ptr, val, order)`**: 将 `val` 原子地写入 `ptr`。
- **`aw_exchange(ptr, val, order)`**: 原子交换，将 `val` 写入 `ptr` 并返回旧值。

#### 2.1.3 比较与交换 (CAS)

- **`aw_cas(ptr, expected_ptr, desired, succ_order, fail_order)`**:
  - 如果 `*ptr == *expected_ptr`，则执行 `*ptr = desired` 并返回 `true`。
  - 否则，将当前 `*ptr` 的值写入 `*expected_ptr` 并返回 `false`。
  - 这是强（Strong）CAS 实现。

#### 2.1.4 算术与位运算 (Fetch-and-Op)

所有操作均返回**操作前**的旧值。

- **`aw_fetch_add(ptr, val, order)`**: 原子加法。
- **`aw_fetch_sub(ptr, val, order)`**: 原子减法。
- **`aw_fetch_and(ptr, val, order)`**: 原子按位与。
- **`aw_fetch_or(ptr, val, order)`**: 原子按位或。
- **`aw_fetch_xor(ptr, val, order)`**: 原子按位异或。

#### 2.1.5 内存屏障

- **`aw_thread_fence(order)`**: 线程级内存屏障。
- **`aw_signal_fence(order)`**: 信号/编译器级屏障（防止指令重排，不产生 CPU 屏障指令）。

------

### 2.3 简化应用 API (`aw_atomic_simple.h`)

此层封装了开发者最常用的同步场景，命名后缀表示语义（`_rlx`, `_acq`, `_rel`, `_ar`）。

#### 2.3.1 读写简化

- `aw_load_acq(ptr)` / `aw_load_rlx(ptr)` / `aw_load_consume(ptr)`
- `aw_store_rel(ptr, val)` / `aw_store_rlx(ptr, val)`

#### 2.3.2 交换与 CAS 简化

- **Swap**: `aw_swap_ar`, `aw_swap_acq`, `aw_swap_rel`, `aw_swap_rlx`。
- **CAS (常用)**:
  - `aw_cas_ar(ptr, exp_ptr, des)`: 成功 AcqRel，失败 Acquire。
  - `aw_cas_acq(ptr, exp_ptr, des)`: 成功 Acquire，失败 Acquire。
  - `aw_cas_rel(ptr, exp_ptr, des)`: 成功 Release，失败 Relaxed。
  - `aw_cas_rlx(ptr, exp_ptr, des)`: 全程 Relaxed。

#### 2.3.3 算术自增/自减简化

- **FAA (Fetch-and-Add)**: `aw_faa_rlx`, `aw_faa_acq`, `aw_faa_rel`, `aw_faa_ar`。
- **FAS (Fetch-and-Sub)**: `aw_fas_rlx`, `aw_fas_acq`, `aw_fas_rel`, `aw_fas_ar`。
- **便捷宏**:
  - `aw_inc_ar(ptr)` / `aw_inc_rlx(ptr)`
  - `aw_dec_ar(ptr)` / `aw_dec_rlx(ptr)`

#### 2.3.4 位运算简化

- **按位与**: `aw_fand_rlx`, `aw_fand_acq`, `aw_fand_rel`, `aw_fand_ar`。
- **按位或**: `aw_for_rlx`, `aw_for_acq`, `aw_for_rel`, `aw_for_ar`。
- **按位异或**: `aw_fxor_rlx`, `aw_fxor_acq`, `aw_fxor_rel`, `aw_fxor_ar`。

#### 2.3.5 屏障简化

- `aw_fence_acq()`: 获取屏障。
- `aw_fence_rel()`: 释放屏障。
- `aw_fence_ar()`: 获取释放屏障。
- `aw_fence_seq()`: 全局顺序一致性屏障。
- `aw_compiler_barrier()`: 编译器屏障（通过 `aw_signal_fence(AW_MO_ACQ_REL)` 实现）。

------

### 2.4. 内部实现说明 

通常不需要直接调用这些宏，但了解其存在有助于调试：

- `_aw_impl_*`: GCC/AC5/AC6 的底层实现前缀。
- `_aw_msvc_*_[8|16|32|64]`: MSVC 下针对不同宽度的显式函数调用。
- `_AW_CAST_[8|16|32|64]`: 用于在 MSVC 中处理 `volatile` 类型强制转换的内部宏。

------

## 3. 支持的编译器与架构

- **GCC / Clang**: 完美支持，利用 `__atomic` 内置函数。
- **MSVC**: 支持 x86/x64，利用 `_Interlocked` 系列指令。
- **ARMCC (AC5 / AC6)**: 完美支持嵌入式开发环境。
- **C11以上**: 自动检测并支持标准 `stdatomic.h`。
