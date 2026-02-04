--- @file awlf/build/toolchains/toolchain_overrides.lua
--- @brief 工具链差异封装
--- @details 聚合各工具链的差异化处理逻辑。

--- 解析 ArmClang 汇编器选择
---@param bindir string|nil 工具链 bin 目录
---@param sdkdir string|nil SDK 根目录
---@return string toolset_as 汇编器名称
local function resolve_armclang_toolset_as(bindir, sdkdir)
    local semver = import("core.base.semver")
    local find_tool = import("lib.detect.find_tool")
    local search_paths = {}
    if bindir and bindir ~= "" then
        search_paths[#search_paths + 1] = bindir
    end
    if sdkdir and sdkdir ~= "" then
        local sdk_bin = path.join(sdkdir, "bin")
        if sdk_bin ~= bindir then
            search_paths[#search_paths + 1] = sdk_bin
        end
    end
    local armclang = find_tool("armclang", {version = true, force = true, paths = search_paths})
    if armclang and armclang.version and semver.compare(armclang.version, "6.13") > 0 then
        return "armclang"
    end
    local armasm = find_tool("armasm", {paths = search_paths})
    if armasm then
        return "armasm"
    end
    return "armclang"
end

--- 补全 ArmClang 的 toolset_as
---@param toolchain_raw string 原始 toolchain 字符串
---@param sdkdir string|nil SDK 根目录
---@param bindir string|nil 工具链 bin 目录
---@return string toolchain_raw 补全后的 toolchain 字符串
local function ensure_armclang_toolset(toolchain_raw, sdkdir, bindir)
    if toolchain_raw and toolchain_raw:find("toolset_as=") then
        return toolchain_raw
    end
    local toolset_as = resolve_armclang_toolset_as(bindir, sdkdir)
    return "armclang[toolset_as=" .. toolset_as .. "]"
end

--- ArmClang 特殊处理入口
---@param selection table 选中信息
---@param paths table 路径信息
---@return string toolchain_raw
local function adjust_armclang(selection, paths)
    return ensure_armclang_toolset(selection.raw, paths.sdkdir, paths.bindir)
end

local toolchain_adjusters = {
    armclang = adjust_armclang,
}

--- 应用工具链差异化处理
---@param selection table 选中信息
---@param paths table 路径信息
---@return string toolchain_raw
function apply_toolchain_overrides(selection, paths)
    local adjuster = toolchain_adjusters[selection.name]
    if adjuster then
        return adjuster(selection, paths)
    end
    return selection.raw
end

return {
    apply_toolchain_overrides = apply_toolchain_overrides,
}
