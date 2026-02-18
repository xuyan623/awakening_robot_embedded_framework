--- @file awlf/build/rules/board_assets.lua
--- @brief AWLF 板级资源规则定义
--- @details 注入板级启动文件与链接脚本。
local awlf_root = path.join(os.scriptdir(), "..", "..")
local modules_root = path.join(awlf_root, "build", "modules")

--- @rule awlf.board_assets
--- @brief 注入板级启动文件与链接脚本
--- @details 配置阶段根据 board/toolchain 解析并注入资源。
rule("awlf.board_assets")
    --- 配置阶段注入板级构建资源
    ---@param target target 目标对象
    on_config(function(target)
        if target:kind() ~= "binary" then
            return
        end
        -- 读取板级构建资源
        local awlf = import("awlf", {rootdir = modules_root})
        local context = awlf.get_context()
        local bsp_root = path.join(awlf_root, "platform", "bsp")
        local bsp = import("bsp", {rootdir = bsp_root})
        local assets = bsp.get_board_build_assets(context.board_name, context.toolchain_name)
        if assets.startup then
            target:add("files", assets.startup)
        end
        local override_sources = bsp.get_board_override_sources(context.board_name)
        if override_sources and #override_sources > 0 then
            local inputs = bsp.get_board_build_inputs(context.board_name)
            target:add("files", override_sources, {
                includedirs = inputs.includedirs,
                defines = inputs.defines,
            })
        end
        if assets.linkerscript then
            local config = import("core.project.config")
            local linker_flag = config.get("toolchain_linker_flag")
            if not linker_flag or linker_flag == "" then
                raise("toolchain linker flag not set: " .. context.toolchain_name)
            end
            target:add("ldflags", linker_flag .. assets.linkerscript, {force = true})
        end
    end)
rule_end()
