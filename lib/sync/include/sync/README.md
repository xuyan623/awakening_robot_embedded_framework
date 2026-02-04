# sync（同步语义层）

`sync` 提供跨平台、可复用的“同步语义原语”，用于在不同执行上下文（任务/线程/ISR）之间表达**等待与完成**等同步关系。

## 职责边界

### sync 做什么
- 提供语义稳定的同步原语与组合原语（例如 `completion`）。
- 依赖 OSAL 提供的最小原语集合（例如 `osal_sem`、`osal_mutex`、`osal_event`）实现跨平台一致语义。
- 对外仅暴露 `awlf/lib/include/sync/` 下的公共 API。

### sync 不做什么
- 不提供“通信语义”（消息、发布订阅、序列化、路由等），这些属于 `services/ipc` 的职责范畴。
- 不直接依赖 `drivers/`、`bsp/` 的业务实现（避免层次倒置与循环依赖）。
- 不在公共头文件中引入任何 RTOS 私有头或类型（例如 FreeRTOS）。

### 与 OSAL / IPC 的关系（简述）
- **OSAL**：抽象操作系统原语（线程、信号量、队列、事件、时间等）。
- **sync**：在 OSAL 之上提供框架所需的同步语义构件（例如 one-shot completion）。
- **services/ipc**：提供通信语义的服务层，可依赖 sync/OSAL，但 sync 不应反向依赖 services/ipc。

## 依赖隔离（硬约束）

为保证长期稳定与跨平台一致性，sync 层必须满足：
- `sync/*.h` 不包含任何 RTOS 私有头，也不暴露 RTOS 私有类型；对外 API 保持稳定。
- 端口/平台定制优化只能出现在实现层（`.c`），并且必须通过构建系统严格控制“何时参与编译”，不得污染其它平台。

## completion 语义
- one-shot 完成信号，单等待者。
- done 先于 wait：wait 立即返回并消费完成信号。
- wait 前重复 done 返回 BUSY（不计数）。
- wait 消费完成信号后自动复位为 INIT 状态。

## completion 的 FreeRTOS 可选优化（任务通知 give/take 计数模式）

### 默认后端（跨平台）
- 默认使用 `osal_sem` 实现（所有端口可用、语义稳定）。
- 通用实现文件：`awlf/lib/source/sync/completion.c`

### FreeRTOS 高性能后端（可选）

启用条件：
- 构建端口为 FreeRTOS（`AWLF_OSAL_PORT=freertos`）。
- 将 `AWLF_COMPLETION_BACKEND` 设为 `freertos_notify`（构建系统会转换为对应宏取值）。

隔离原则（必须满足）：
- `sync/completion.h` 不包含 `FreeRTOS.h` / `task.h`，不暴露 FreeRTOS 类型。
- 仅在 FreeRTOS 专用实现文件中包含 FreeRTOS 头：
  - `awlf/platform/osal/freertos/sync/completion_notify.c`
- 该文件只在 “FreeRTOS 构建 + 宏启用” 时参与编译；其它平台继续编译通用实现，完全不接触 FreeRTOS。

资源约定：
- 该后端会占用一个 task notification index：
  - `AWLF_FREERTOS_COMPLETION_NOTIFY_INDEX`（默认 `0`）
- 若使用非 0 的 index，必须保证 `configTASK_NOTIFICATION_ARRAY_ENTRIES > AWLF_FREERTOS_COMPLETION_NOTIFY_INDEX`。
- 该 index 必须在全局范围内保持唯一，避免被其它模块复用（否则会发生逻辑冲突）。

### XMake 构建示例（用于工程化验证）
- Host(POSIX)：
  - `xmake f -c --profile=host-linux-gcc-posix-debug`
  - `xmake awlf quality verify`
- FreeRTOS（默认 profile）：
  - `xmake f -c --profile=rm-c-board-stm32f407ighx-armgcc-freertos`
  - `xmake awlf quality verify`

> 说明：若要启用 FreeRTOS notify 优化，请在 `buildspec/xmake.lua` 的对应 profile 中配置 `AWLF_COMPLETION_BACKEND` 与 `AWLF_FREERTOS_COMPLETION_NOTIFY_INDEX` 的宏定义，然后使用该 profile 构建/验证。

## 端口扩展指南（新增优化的标准做法）
- 新增端口定制实现放置在：`awlf/platform/osal/<port_name>/sync/`
- 只在对应端口构建时将实现文件加入编译（通过构建系统选择），避免“实现可见但不可用”的平台污染。
- 对外 API 与类型始终放在 `awlf/lib/include/sync/` 并保持跨平台一致。

