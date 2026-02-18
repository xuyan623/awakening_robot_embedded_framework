--- @file awlf/build/modules/awlf.lua
--- @brief AWLF 上下文与信息输出模块
--- @details 负责持久化配置上下文、只读获取上下文，并输出构建信息。

--- AWLF 基础信息
---@class AwlfInfo
---@field name string 框架名称
---@field version string 版本号
---@field author string 作者
---@field description string 描述
---@field license string 许可
local awlf = {
    name = "awlf",
    version = "0.1.0",
    author = "Bamboo",
    description = "A lightweight framework for embedded systems",
    license = "MIT",
}

--- 上下文键名列表（同时用于校验必需项）
---@type string[]
local CONTEXT_KEYS = {
    "board_name",
    "chip_name",
    "os_name",
    "toolchain_name",
    "arch",
    "arch_flags",
    "toolchain_flags",
    "board_os_config_dir",
}

--- 上下文缓存
---@type table|nil
local context_cache = nil

--- 预设缓存（awlf_preset.lua）
local preset_cache = nil
local preset_root_cache = nil

--- 解析预设根目录（项目根目录）
---@return string root 根目录路径
local function resolve_preset_root()
    if preset_root_cache then
        return preset_root_cache
    end
    local root = os.projectdir()
    if not root or root == "" then
        raise("project root not found: os.projectdir() returned empty")
    end
    preset_root_cache = root
    return root
end

--- 获取预设根目录
---@return string root 根目录路径
function get_preset_root()
    return resolve_preset_root()
end

--- 获取用户预设（awlf_preset.lua）
---@return table|nil preset 预设配置
function get_preset()
    if preset_cache == false then
        return nil
    end
    if preset_cache then
        return preset_cache
    end
    local root = resolve_preset_root()
    local preset_path = path.join(root, "awlf_preset.lua")
    if not os.isfile(preset_path) then
        preset_cache = false
        return nil
    end
    local module = import("awlf_preset", {rootdir = root})
    local preset = nil
    --- 判断表是否为空
    ---@param value table|nil
    ---@return boolean empty
    local function is_empty_table(value)
        for _ in pairs(value or {}) do
            return false
        end
        return true
    end
    if type(module) == "table" then
        if type(module.get_preset) == "function" then
            preset = module.get_preset()
        elseif type(module.awlf_preset) == "table" then
            preset = module.awlf_preset
        elseif type(module.preset) == "table" then
            preset = module.preset
        elseif not is_empty_table(module) then
            preset = module
        end
    end
    if type(preset) ~= "table" then
        raise("awlf_preset invalid: expected table or get_preset()")
    end
    preset_cache = preset
    return preset_cache
end

--- 确保上下文缓存已初始化
---@return table context 上下文缓存表
local function ensure_context()
    if not context_cache then
        context_cache = {}
    end
    return context_cache
end

--- 持久化上下文到配置缓存
---@param values table|nil 上下文键值对
---@return nil
local function persist_context_values(values)
    local config = import("core.project.config")
    local should_save = false
    --- 判断值是否可持久化
    ---@param value any
    ---@return boolean persistable
    local function is_persistable(value)
        local value_type = type(value)
        return value_type == "string"
            or value_type == "number"
            or value_type == "boolean"
            or value_type == "table"
    end
    for key, value in pairs(values or {}) do
        if is_persistable(value) then
            config.set("awlf_ctx_" .. key, value)
            should_save = true
        end
    end
    if should_save then
        config.save()
    end
end

--- 设置单个上下文键并持久化
---@param key string 上下文键
---@param value any 上下文值
---@return nil
local function set_context_value(key, value)
    local context = ensure_context()
    context[key] = value
    persist_context_values({[key] = value})
end

--- 设置多个上下文键并持久化
---@param values table|nil 上下文键值对
---@return nil
function set_context_extra(values)
    local context = ensure_context()
    for key, value in pairs(values or {}) do
        if value ~= nil then
            context[key] = value
        end
    end
    persist_context_values(values)
end

--- 从配置缓存加载上下文
---@return nil
local function load_context_from_config()
    local context = {}
    local config = import("core.project.config")
    for _, key in ipairs(CONTEXT_KEYS) do
        local value = config.get("awlf_ctx_" .. key)
        if value and value ~= "" then
            context[key] = value
        end
    end
    context_cache = context
end

--- 同步上下文与当前配置
---@return boolean synced 是否执行了同步
function sync_context_from_config()
    local config = import("core.project.config")
    config.load()
    local board_name = config.get("board")
    if not board_name or board_name == "" then
        return false
    end
    local os_name = config.get("os")
    local modules_root = os.scriptdir()
    local awlf_root = path.join(os.scriptdir(), "..", "..")
    local bsp_root = path.join(awlf_root, "platform", "bsp")
    local toolchain_lib = import("awlf_toolchain_lib", {rootdir = modules_root})
    local toolchain_context = toolchain_lib.resolve_toolchain_context()
    toolchain_lib.apply_defaults(toolchain_context)
    local toolchain_name = toolchain_context.toolchain_name
    local bsp = import("bsp", {rootdir = bsp_root})
    local profile = bsp.get_profile(board_name)
    local arch = bsp.resolve_arch(profile)
    local arch_traits = bsp.get_arch_traits(profile)
    local platform_arch = toolchain_lib.resolve_platform_arch(
        toolchain_context.toolchain_config,
        toolchain_name,
        arch
    )
    toolchain_lib.apply_platform_arch(platform_arch)
    toolchain_lib.validate_arch_traits(arch_traits)
    local arch_flags = toolchain_lib.map_arch_flags(
        toolchain_context.toolchain_config,
        toolchain_name,
        arch_traits
    )
    local toolchain_flags = toolchain_lib.get_flags(
        toolchain_context.toolchain_config,
        toolchain_name
    )
    local context_values = {
        toolchain_name = toolchain_name,
        board_name = board_name,
        chip_name = profile.board.chip,
        arch = arch,
        arch_flags = arch_flags,
        toolchain_flags = toolchain_flags,
    }
    if os_name and os_name ~= "" then
        context_values.os_name = os_name
    end
    set_context_extra(context_values)
    if os_name and os_name ~= "" then
        bsp.update_board_os_context({set_context_extra = set_context_extra}, board_name, os_name)
    end
    return true
end

--- 获取上下文（只读），缺失时直接报错
---@return table context 上下文缓存表
function get_context()
    if not context_cache then
        load_context_from_config()
    end
    local missing = {}
    for _, key in ipairs(CONTEXT_KEYS) do
        if not context_cache[key] or context_cache[key] == "" then
            missing[#missing + 1] = key
        end
    end
    if #missing > 0 then
        raise("context missing: " .. table.concat(missing, ", ")
            .. ". Please run `xmake f -c --board=<name> --os=<name> -m <mode>` first.")
    end
    return context_cache
end

--- 输出 AWLF 信息与当前上下文摘要
---@return nil
function awlf_print_info()
    --- 输出单个字段，缺失时报错
    ---@param label string 字段名
    ---@param value string|number|boolean|nil 字段值
    ---@return nil
    local function print_field(label, value)
        if not value then
            raise("[awlf] " .. label .. ": none")
        end
        print("[awlf] " .. label .. ": " .. value)
    end

    print("[awlf] name: " .. awlf.name)
    print("[awlf] version: " .. awlf.version)
    print("[awlf] author: " .. awlf.author)
    print("[awlf] description: " .. awlf.description)
    print("[awlf] license: " .. awlf.license)
    local context = get_context()
    print_field("board", context.board_name)
    print_field("chip", context.chip_name)
    print_field("toolchain", context.toolchain_name)
    print_field("os", context.os_name)
end

--- 解析构建模式（优先使用 target 官方接口）
---@param target target|nil 目标对象
---@return string mode 构建模式
local function resolve_build_mode(target)
    if target and type(target.is_mode) == "function" then
        if target:is_mode("debug") then
            return "debug"
        end
        if target:is_mode("release") then
            return "release"
        end
    end
    local config = import("core.project.config")
    local mode = config.get("mode")
    if type(mode) == "string" and mode ~= "" then
        return mode
    end
    return "unknown"
end

--- 归一化内存分布行
---@param name string 区域名
---@param used integer|nil 已用字节
---@param total integer|nil 总字节
---@return string line 输出行
local function format_memory_distribution_line(name, used, total)
    if type(used) ~= "number" then
        return string.format("[awlf]   %-5s: unknown", name)
    end
    if type(total) == "number" and total > 0 then
        local percent = (used * 100.0) / total
        local percent_text = string.format("%.2f", percent) .. "%%"
        return string.format("[awlf]   %-5s: %d / %d (%s)", name, used, total, percent_text)
    end
    return string.format("[awlf]   %-5s: %d / unknown", name, used)
end

--- 输出构建摘要（构建模式与内存分布）
---@param target target|nil 目标对象
---@return nil
function awlf_print_build_profile(target)
    local build_mode = resolve_build_mode(target)
    print("[awlf] build mode: " .. build_mode)
    local modules_root = os.scriptdir()
    local toolchain_lib = import("awlf_toolchain_lib", {rootdir = modules_root})
    local report = toolchain_lib.build_memory_distribution_report(target, get_context())
    print("[awlf] memory distribution:")
    if not report or report.available == false then
        local reason = report and report.reason or "unknown"
        print("[awlf]   unavailable: " .. tostring(reason))
        return
    end
    print(format_memory_distribution_line("FLASH", report.flash_used, report.flash_total))
    print(format_memory_distribution_line("RAM", report.ram_used, report.ram_total))
    if report.reason and report.reason ~= "" then
        print("[awlf]   note: " .. report.reason)
    end
end

--- 校验链接契约（关键符号必须由指定模块提供）
---@param target target|nil 目标对象
---@return table report 校验结果
function awlf_verify_link_contract(target)
    local modules_root = os.scriptdir()
    local toolchain_lib = import("awlf_toolchain_lib", {rootdir = modules_root})
    return toolchain_lib.verify_awlf_link_contract(target, get_context())
end

return {
    get_preset_root = get_preset_root,
    get_preset = get_preset,
    set_context_extra = set_context_extra,
    get_context = get_context,
    awlf_print_info = awlf_print_info,
    awlf_print_build_profile = awlf_print_build_profile,
    awlf_verify_link_contract = awlf_verify_link_contract,
    sync_context_from_config = sync_context_from_config,
}
