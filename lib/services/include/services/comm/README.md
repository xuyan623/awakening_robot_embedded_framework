# comm

`services/comm` 提供公共通信语义抽象（例如 Pub/Sub、Req/Rep、背压与可靠性策略）。

边界约束：
- `comm core` 仅依赖 `core/osal/sync`，不直接依赖具体总线驱动实现。
- `comm adapter` 位于实现端（例如 `drivers/src/peripheral/<bus>/comm_adapter_*`），依赖 `services/comm` 抽象完成接入。
- 分层总依赖矩阵以 `awlf/document/architecture/分层与依赖规范.md` 为准。
