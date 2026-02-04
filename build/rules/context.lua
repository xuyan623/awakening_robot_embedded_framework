--- @file awlf/build/rules/context.lua
--- @brief AWLF 上下文规则定义
--- @details 注入架构/工具链参数并执行构建前校验。
local awlf_root = path.join(os.scriptdir(), "..", "..")
local modules_root = path.join(awlf_root, "build", "modules")

--- @rule awlf.context
--- @brief 注入上下文编译/链接参数
--- @details 配置阶段读取上下文并向目标注入编译标志。
rule("awlf.context")
    --- 规则配置阶段读取上下文并注入编译标志
    ---@param target target 目标对象
    on_config(function(target)
        -- 读取上下文与工具链配置
        local toolchain_lib = import("awlf_toolchain_lib", {rootdir = modules_root})
        local awlf = import("awlf", {rootdir = modules_root})
        awlf.sync_context_from_config()
        local context = awlf.get_context()
        local arch_flags = context.arch_flags
        local toolchain_flags = context.toolchain_flags
        local toolchain_config = toolchain_lib.get_toolchain_config()
        local include_arch_ldflags = toolchain_lib.should_include_arch_ldflags(toolchain_config, context.toolchain_name)
        -- 注入架构参数与工具链参数
        toolchain_lib.add_target_flag_set(target, arch_flags, {
            force = true,
            include_ldflags = include_arch_ldflags and (target:kind() == "binary"),
        })
        toolchain_lib.add_target_flag_set(target, toolchain_flags, {
            force = false,
            include_ldflags = (target:kind() == "binary"),
        })
        -- 校验工具链与硬浮点支持
        toolchain_lib.ensure_toolchain_checked(context.toolchain_name)
        toolchain_lib.validate_hard_float_support(target, arch_flags.cflags, context.toolchain_name)
    end)
rule_end()
