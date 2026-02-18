# OSAL+SYNC进度说明

## 0. 维护规则（强制）

- 每次阶段性验收完成后，必须更新本文档。
- 对同一功能的后续重构或修订，必须在原有条目原地更新，不得另起一行规避历史结论。
- 每次更新必须包含：阶段、变更范围、当前状态、验证结果、未解决问题。
- 文档记录必须可由代码路径或验证结果回溯，不得写入不可验证结论。

## 1. 适用范围与依据

- 适用范围：`awlf/lib/osal` 与 `awlf/lib/sync` 相关重构进度。
- 规划依据：
  - `awlf/lib/osal/doc/OSAL+SYNC工程落地规划.md`
  - `awlf/lib/osal/doc/OSAL+SYNC重构计划.md`
  - `awlf/lib/osal/doc/OSAL+SYNC顶层设计指南ForAGENTS.md`
- 当前代码参考（time 子域）：
  - `awlf/lib/osal/include/osal/osal_time.h`
  - `awlf/platform/osal/freertos/osal_time_freertos.c`
  - `awlf/platform/osal/freertos/osal_time_freertos.h`
  - `awlf/samples/stm32f407xx/osal_time/main.c`

## 2. 阶段总览（按阶段原地维护）

| 阶段 | 验收结论 | 当前状态 | 代码证据 | 验证状态 | 未解决问题 |
| --- | --- | --- | --- | --- | --- |
| Phase 0 | 已阶段验收（按既定“规划-确认-执行”完成） | 已冻结，进入维护态 | `awlf/lib/osal/doc/OSAL+SYNC重构计划.md`、`awlf/lib/osal/doc/OSAL+SYNC顶层设计指南ForAGENTS.md` | 已完成边界冻结与阶段确认 | 需继续按阶段补齐“验收证据可追溯”记录 |
| Phase 1 | 部分验收通过（time + core + thread + semaphore + mutex + queue + event_flags + timer 子域机制重构完成） | 进行中（time/core/thread/semaphore/mutex/queue/event_flags/timer 已落地） | `awlf/lib/osal/include/osal/osal_time.h`、`awlf/platform/osal/freertos/osal_time_freertos.c`、`awlf/platform/osal/freertos/osal_time_freertos.h`、`awlf/samples/stm32f407xx/osal_time/main.c`、`awlf/lib/osal/include/osal/osal_core.h`、`awlf/platform/osal/freertos/osal_core_freertos.c`、`awlf/lib/osal/include/osal/osal_queue.h`、`awlf/platform/osal/freertos/osal_queue_freertos.c`、`awlf/samples/stm32f407xx/osal_queue/main.c`、`awlf/lib/osal/include/osal/osal_event.h`、`awlf/platform/osal/freertos/osal_event_freertos.c`、`awlf/samples/stm32f407xx/osal_event/main.c`、`awlf/lib/source/sync/completion.c`、`awlf/platform/sync/freertos/completion_notify.c`、`awlf/lib/osal/include/osal/osal_thread.h`、`awlf/platform/osal/freertos/osal_thread_freertos.c`、`awlf/lib/osal/include/osal/osal_sem.h`、`awlf/platform/osal/freertos/osal_sem_freertos.c`、`awlf/lib/osal/include/osal/osal_mutex.h`、`awlf/platform/osal/freertos/osal_mutex_freertos.c`、`awlf/lib/osal/include/osal/osal_timer.h`、`awlf/platform/osal/freertos/osal_timer_freertos.c`、`awlf/samples/stm32f407xx/osal_timer/main.c` | `xmake build robot_project` 已通过 | OSAL 其他原语待逐项审计与机制重构；需持续跟踪非 1kHz tick 等价性与后续新增线程栈参数是否遵循字节语义 |
| Phase 2 | 待验收 | 待开始 | 待补录 | 待补录 | 待分原语逐项审计 |
| Phase 3 | 待验收 | 待开始 | 待补录 | 待补录 | 待分原语逐项审计 |
| Phase 4 | 待验收 | 待开始 | 待补录 | 待补录 | 待补录 |
| Phase 5 | 待验收 | 待开始 | 待补录 | 待补录 | 待补录 |

## 3. 当前已确认进展（与阶段总览一一对应）

### 3.1 Phase 0（架构冻结与工程骨架）

- 验收结论：已阶段验收。
- 本阶段定位：冻结边界与执行模式，不在本阶段引入大规模机制改造。
- 当前状态：维护态；后续仅允许按验收结论原地修订，不新增并行版本描述。

### 3.2 Phase 1（OSAL 最小原语）

- 验收结论：部分验收通过（time + core + thread + semaphore + mutex + queue + event_flags + timer 子域）。
- 已落地实现进度（time）：
  - `osal_delay_until` 参数语义已统一为 in/out 游标 `deadline_cursor_ms`。
  - 周期延时主路径优先使用 `xTaskDelayUntil`，并保留宏关闭时回退路径。
  - `osal_time` 样例命名已统一为游标语义（`deadline_cursor_ms` / `expected_previous_deadline_ms`）。
- 已落地实现进度（core）：
  - `osal_malloc/osal_free` 已明确严格模式：禁止在 ISR 中调用。
  - 误用处理策略已落地：Debug 断言 + 运行时安全返回（`osal_malloc` 返回 `NULL`，`osal_free` 直接返回）。
  - 已拆分临界区接口为 task/isr 两套语义：`osal_irq_lock_task/unlock_task` 与 `osal_irq_lock_from_isr/unlock_from_isr`。
  - 已移除旧的魔数编码路径（`0x80000000u/0x7FFFFFFFu`），并完成现有调用点迁移。
- 已落地实现进度（thread）：
  - `osal_thread_attr_t.stack_size` 已统一为“字节”语义，FreeRTOS 端内部完成字节到 `configSTACK_DEPTH_TYPE` 的转换。
  - `osal_thread_create` 已收敛为 `thread` 必填（不可为 `NULL`），并补齐参数校验与失败时句柄清零。
  - `osal_thread_join` 维持 `OSAL_NOT_SUPPORTED`，但已补齐线程上下文与参数合法性校验。
  - `osal_thread_create/join/terminate/yield/exit/kernel_start` 已统一线程上下文约束（ISR 误用：断言 + 安全返回）。
  - `osal_thread_terminate` 已保留 `terminate(other)` 机制语义，并完成风险标注：不承诺自动回收目标线程持有的业务资源。
  - `terminate(self)` 已收敛为 `OSAL_INVALID`，强制改用 `osal_thread_exit` 自退出。
  - 新增独立样例：`awlf/samples/stm32f407xx/osal_thread/main.c`，覆盖 create/self/yield/join/terminate 核心边界。
  - `awlf/samples/stm32f407` 下已完成线程栈参数迁移：历史按 word 编写的栈配置已显式换算为字节表达（`* OSAL_STACK_WORD_BYTES`）。
- 已落地实现进度（semaphore）：
  - `osal_sem_wait(OSAL_WAIT_FOREVER)` 保持无限等待语义，不做降级。
  - `osal_sem_post/osal_sem_post_from_isr` 在计数已满时统一返回 `OSAL_NO_RESOURCE`。
  - 新增计数查询接口：`osal_sem_get_count` 与 `osal_sem_get_count_from_isr`。
  - `create/delete/wait/post/get_count` 与 `post_from_isr/get_count_from_isr` 上下文边界已收敛（误用：断言 + 安全返回）。
  - `osal_sync` 样例已补充 semaphore 边界验证：`get_count` 与满计数 `post` 行为。
- 已落地实现进度（mutex）：
  - `mutex` 合同已收敛为非递归语义：同线程重复加锁按超时规则处理，不归类为参数非法。
  - `mutex` 解锁语义已收敛：非 owner 解锁返回 `OSAL_INVALID`。
  - `create/delete/lock/unlock` 上下文边界已收敛（线程上下文；误用：断言 + 安全返回）。
  - `osal_sync` 样例已补充 mutex 边界验证：未持有即解锁返回 `OSAL_INVALID`。
- 已落地实现进度（queue）：
  - `queue` 查询接口已状态化：`messages_waiting/spaces_available` 改为 `OSAL status + out 参数`，消除“0 值混淆错误”的接口歧义。
  - FreeRTOS 后端已移除本地 `registry` 影子状态，查询主路径优先采用官方接口。
  - `send/recv/peek` 的 ISR 版本已收敛为上下文强约束（误用：断言 + 安全返回）。
  - ISR 满/空路径返回码已收敛为业务语义 `OSAL_WOULD_BLOCK`，不再错误归类为 `OSAL_INTERNAL`。
  - 新增独立样例：`awlf/samples/stm32f407xx/osal_queue/main.c`，用于 queue 合同边界验证。
- 已落地实现进度（event_flags）：
  - `create/delete/set/clear/wait` 已统一线程上下文约束（误用：断言 + 安全返回）。
  - `set_from_isr` 已收敛为 ISR-only 合同（误用：断言 + 安全返回）。
  - `set_from_isr` 失败路径返回码已收敛为 `OSAL_NO_RESOURCE`，不再笼统归类 `OSAL_INTERNAL`。
  - 新增 `OSAL_EVENT_FLAGS_USABLE_MASK` 合同，接口固定为 `uint32_t`；公共头不再提供默认兜底，必须由端口层注入。
  - FreeRTOS 端已在 `awlf/platform/osal/freertos/xmake.lua` 注入 `AWLF_OSAL_EVENT_FLAGS_USABLE_MASK=0x00FFFFFFu`。
  - FreeRTOS 后端已统一对 `set/clear/wait/set_from_isr` 输入位进行掩码校验，超出掩码返回 `OSAL_INVALID`。
  - `wait` 输出值已收敛为仅返回可用业务位，屏蔽底层控制位。
  - 独立样例 `awlf/samples/stm32f407xx/osal_event/main.c` 已补充非法高位输入验证，并保留 `wait_any/wait_all/no_clear/timeout` 核心语义覆盖。
- 已落地实现进度（timer）：
  - `osal_timer_create` 已升级为 `status + out_handle` 形态，去除“句柄或 NULL”混合语义。
  - 新增 `osal_timer_mode_t`，明确 `one-shot/periodic` 模式语义，替代裸 `auto_reload` 参数。
  - 新增 `osal_timer_reset`，并固定“重启/重装（rearm）”合同：未运行态 reset 等价 start。
  - `create/start/stop/reset/delete/get_id/set_id` 已统一线程上下文约束（ISR 误用：断言 + 安全返回）。
  - `start/stop/reset/delete` 命令未入队时的返回码已收敛为等待语义映射：
    `timeout=0 -> OSAL_WOULD_BLOCK`，`timeout>0/OSAL_WAIT_FOREVER -> OSAL_TIMEOUT`。
  - 新增独立样例：`awlf/samples/stm32f407xx/osal_timer/main.c`，覆盖 create/start/stop/reset/delete/get_id/set_id 核心边界。
- 未完成范围：
  - `join` 真实等待后端仍未落地（当前 FreeRTOS 端为 `OSAL_NOT_SUPPORTED`）；OSAL 其余原语仍待逐项审计与机制重构。
- 待做事项（下一项）：
  - thread join 语义重构（合同冻结 -> FreeRTOS 可行性收敛 -> 样例边界验证）。

### 3.3 Phase 2（SYNC reference）

- 当前状态：待开始。
- 细化进度：待补录。

### 3.4 Phase 3（selector 与 capability）

- 当前状态：待开始。
- 细化进度：待补录。

### 3.5 Phase 4（双端一致性验证）

- 当前状态：待开始。
- 细化进度：待补录。

### 3.6 Phase 5（可选优化）

- 当前状态：待开始。
- 细化进度：待补录。

## 4. 更新模板（后续阶段直接覆盖原条目）

> 说明：第 3 章的各 Phase 小节必须与第 2 章一一对应，且仅做原地更新，不新增并行记录。

| 阶段 | 功能域 | 当前状态 | 代码证据 | 验证状态 | 未解决问题 |
| --- | --- | --- | --- | --- | --- |
| Phase X | 示例：OSAL queue | 示例：进行中 | 示例：`awlf/platform/osal/freertos/osal_queue_freertos.c` | 示例：构建通过/用例通过 | 示例：ISR 路径待压测 |
