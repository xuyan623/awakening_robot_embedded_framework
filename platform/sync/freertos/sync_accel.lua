--- @file awlf/platform/sync/freertos/sync_accel.lua
--- @brief FreeRTOS 同步加速后端信息
--- @details 提供加速实现所需的头文件目录与编译宏。

--- 获取加速后端信息
---@return table accel_info 加速信息
function get_accel_info()
    local awlf = import("awlf")
    local context = awlf.get_context()

    local os_root = path.join(os.scriptdir(), "..", "..", "osal", context.os_name)
    local include_dir = path.join(os_root, "FreeRTOS", "include")
    if not os.isdir(include_dir) then
        raise("FreeRTOS include dir not found: " .. include_dir)
    end
    local board_config_dir = context.board_os_config_dir
    local portable_dir = path.join(os_root, "portable", context.toolchain_name, context.arch)

    return {
        include_dirs = { include_dir, board_config_dir, portable_dir },
        defines = {
            "AWLF_SYNC_ACCEL=1",
            "AWLF_SYNC_ACCEL_COMPLETION=1",
        },
    }
end
