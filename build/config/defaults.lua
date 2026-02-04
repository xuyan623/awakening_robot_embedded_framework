--- @file awlf/build/config/defaults.lua
--- @brief AWLF 默认构建选项
--- @details 定义 board/os/sync_accel 的默认选择。

--- 默认选项结构
---@class AwlfDefaults
---@field board string 默认板级名称
---@field os string 默认操作系统名称
---@field sync_accel string 默认同步加速策略（auto/none）

--- 默认选项实例
---@type AwlfDefaults
awlf_defaults = {
    board = "rm-c-board",
    os = "freertos",
    sync_accel = "auto",
}
