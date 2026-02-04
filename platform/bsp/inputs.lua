--- @file awlf/platform/bsp/inputs.lua
--- @brief BSP 构建输入组装
--- @details 负责整合 board/chip/vendor 输入并规范化路径。
local bsp_root = os.scriptdir()
local utils = import("utils", {rootdir = bsp_root})
local data_loader = import("data_loader", {rootdir = bsp_root})
local components = import("components", {rootdir = bsp_root})

--- 获取板级构建输入（头文件、源文件、宏与额外资源）
---@param board_name string 板级名称
---@return table inputs 构建输入
function get_board_build_inputs(board_name)
    local profile = data_loader.get_profile(board_name)

    local inputs = {
        defines = {},
        includedirs = {},
        sources = {},
        headerfiles = {},
        extrafiles = {},
    }

    utils.append_list(inputs.defines, profile.vendor.defines)
    utils.append_list(inputs.defines, profile.chip.defines)
    utils.append_list(inputs.defines, profile.board.defines)

    utils.append_list(inputs.includedirs, profile.vendor.includedirs)
    utils.append_list(inputs.includedirs, profile.chip.includedirs)
    utils.append_list(inputs.includedirs, profile.board.includedirs)

    utils.append_list(inputs.sources, profile.vendor.sources)
    utils.append_list(inputs.sources, profile.chip.sources)
    utils.append_list(inputs.sources, profile.board.sources)

    local component_data = components.collect_component_data(profile)
    utils.append_list(inputs.defines, component_data.defines)
    utils.append_list(inputs.includedirs, component_data.includedirs)
    utils.append_list(inputs.sources, component_data.sources)
    utils.append_list(inputs.headerfiles, component_data.headerfiles)
    utils.append_list(inputs.extrafiles, component_data.extrafiles)

    inputs.includedirs = utils.normalize_paths(bsp_root, inputs.includedirs)
    inputs.sources = utils.normalize_paths(bsp_root, inputs.sources)
    inputs.headerfiles = utils.normalize_paths(bsp_root, inputs.headerfiles)
    inputs.extrafiles = utils.normalize_paths(bsp_root, inputs.extrafiles)

    return inputs
end
