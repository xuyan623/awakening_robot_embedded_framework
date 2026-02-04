--- @file awlf/platform/osal/xmake.lua
--- @brief OSAL 构建脚本
--- @details 负责 OSAL 统一目标与 OS 路由。

local osal_root = os.scriptdir()

local os_name = get_config("os")

if os_name and os_name ~= "" then
    local os_dir = path.join(osal_root, os_name)
    if os.isdir(os_dir) then
        includes(os_dir)
    end
end

--- @target tar_osal
--- @brief OSAL 聚合静态库
--- @details 汇总 OS 端口并依赖 OSAL API。
target("tar_osal")
    set_kind("static")
    add_deps("tar_awapi_osal", {public = true})
    add_deps("tar_os")
    add_rules("awlf.context")
target_end()
