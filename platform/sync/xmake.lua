--- @file awlf/platform/sync/xmake.lua
--- @brief SYNC 构建脚本
--- @details 负责同步原语默认实现与加速后端的编译注入。

local sync_root = os.scriptdir()
local lib_root = path.join(sync_root, "..", "..", "lib")
local sync_source_glob = path.join(lib_root, "source", "sync", "*.c")

--- @target tar_sync
--- @brief 同步原语静态库
--- @details 注入同步原语与可选加速后端实现。
target("tar_sync")
    set_kind("static")
    add_deps("tar_awapi_sync", {public = true})
    add_deps("tar_awcore", {public = true})
    add_deps("tar_osal", {public = true})
    add_rules("awlf.context")
    add_files(sync_source_glob)
    --- 配置阶段选择加速后端并注入编译项
    ---@param target target 目标对象
    on_config(function(target)
        -- 读取上下文与同步加速策略
        local awlf = import("awlf")
        local sync_accel_lib = import("sync_accel_lib", {rootdir = sync_root})
        local context = awlf.get_context()
        local os_name = context.os_name
        local accel_mode = get_config("sync_accel") or "auto"
        if accel_mode ~= "auto" and accel_mode ~= "none" then
            raise("sync_accel invalid: " .. accel_mode)
        end

        local os_dir = path.join(sync_root, os_name)
        local accel_files = os.files(path.join(os_dir, "*.c"))
        local accel_enabled = (accel_mode == "auto") and (#accel_files > 0)
        local accel_info = nil

        if accel_enabled then
            local err = nil
            accel_info, err = sync_accel_lib.try_get_accel_info(os_dir)
            if not accel_info then
                if accel_mode == "auto" then
                    accel_enabled = false
                else
                    raise(err)
                end
                print(err)
            end
        end

        if accel_enabled then
            target:add("files", accel_files)
            if accel_info and accel_info.include_dirs then
                for _, include_dir in ipairs(accel_info.include_dirs) do
                    target:add("includedirs", include_dir, {public = false})
                end
            end
            if accel_info and accel_info.defines then
                target:add("defines", accel_info.defines)
            end
            return
        end

        target:add("defines", "AWLF_SYNC_ACCEL=0")
        target:add("defines", "AWLF_SYNC_ACCEL_COMPLETION=0")
    end)
target_end()
