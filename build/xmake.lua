--- @file awlf/build/xmake.lua
--- @brief AWLF 构建入口
--- @details 负责加载构建配置、规则、工具链与模块入口。
add_moduledirs("modules")

includes("toolchains")
add_rules("awlf.toolchain_bootstrap")
includes("config")
includes("rules")
includes("tasks")

includes("../lib")
includes("../platform/osal")
includes("../platform/sync")
includes("../platform/bsp")

--- @target tar_awlf
--- @brief AWLF 聚合静态库
--- @details 汇总核心、算法、驱动、系统、BSP、OSAL 与同步模块。
target("tar_awlf")
    set_kind("static")
    add_deps("tar_awcore", {public = true})
    add_deps("tar_awalgo", {public = true})
    add_deps("tar_awdrivers", {public = true})
    add_deps("tar_awsystems", {public = true})
    add_deps("tar_bsp", {public = true})
    add_deps("tar_osal", {public = true})
    add_deps("tar_sync", {public = true})
    -- 构建完成后输出 AWLF 信息
    ---@param target target 目标对象
    after_build(function(target)
        import("awlf")
        awlf.awlf_print_info()
    end)
target_end()
