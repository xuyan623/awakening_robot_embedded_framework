--- @file awlf/lib/xmake.lua
--- @brief AWLF lib 子库构建脚本
--- @details 按单一责任原则拆分 core/algorithm/drivers/systems 等静态库。

--- @target tar_awcore
--- @brief AW 核心静态库
--- @details 提供核心基础能力与数据结构。
target("tar_awcore")
    set_kind("static")
    add_rules("awlf.context", {public = true})
    add_includedirs("include", {public = true})
    add_files("source/aw_core/**.c")
    add_files("source/data_struct/**.c")
target_end()

--- @target tar_awapi_osal
--- @brief OSAL API 头文件接口
--- @details 为 OSAL 目标提供公共头文件。
target("tar_awapi_osal")
    set_kind("headeronly")
    add_deps("tar_awcore", {public = true})
    add_includedirs("osal/include", {public = true})
target_end()

--- @target tar_awapi_sync
--- @brief Sync API 头文件接口
--- @details 为同步原语目标提供公共头文件。
target("tar_awapi_sync")
    set_kind("headeronly")
    add_includedirs("sync/include", {public = true})
target_end()

--- @target tar_awapi_pal
--- @brief PAL API 头文件接口
--- @details 为驱动层提供公共头文件与依赖。
target("tar_awapi_pal")
    set_kind("headeronly")
    add_deps("tar_awcore", {public = true})
    add_includedirs("drivers/include", {public = true})
    add_deps("tar_awapi_osal", {public = true})
    add_deps("tar_awapi_sync", {public = true})
target_end()

--- @target tar_awalgo
--- @brief 算法静态库
--- @details 提供算法相关实现。
target("tar_awalgo")
    set_kind("static")
    add_rules("awlf.context")
    add_includedirs("include", {public = true})
    add_files("source/algorithm/**.c")
target_end()

--- @target tar_awdrivers
--- @brief 驱动静态库
--- @details 聚合驱动层实现与依赖。
target("tar_awdrivers")
    set_kind("static")
    add_deps("tar_awapi_pal", {public = true})
    add_rules("awlf.context")
    add_files("drivers/src/**.c")
target_end()

--- @target tar_awsystems
--- @brief 系统静态库
--- @details 提供系统级实现与公共头文件。
target("tar_awsystems")
    set_kind("static")
    add_deps("tar_awcore", {public = true})
    add_includedirs("include", {public = true})
    add_includedirs("systems/include", {public = true})
    add_files("systems/src/**.c")
target_end()
