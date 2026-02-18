--- @file awlf/platform/osal/linux/xmake.lua
--- @brief Linux OSAL 端口骨架
--- @details Phase 0 仅建立目录骨架，不提供可用实现。

target("tar_os")
    set_kind("static")
    on_load(function()
        raise("linux osal port is skeleton-only in Phase 0. Do not enable linux os index yet.")
    end)
target_end()
