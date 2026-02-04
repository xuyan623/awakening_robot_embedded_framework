--- @file awlf/platform/bsp/xmake.lua
--- @brief BSP 构建路由脚本
--- @details 负责加载板级模块并汇总 BSP 目标依赖。

--- 解析板级脚本目录
---@param board_name string 板级名称
---@return string|nil board_dir 板级目录
local function resolve_board_dir(board_name)
    local internal_dir = path.join("boards", board_name)
    if os.isdir(internal_dir) then
        return internal_dir
    end
    return nil
end

local board_name = get_config("board")
if board_name and board_name ~= "" then
    local board_dir = resolve_board_dir(board_name)
    if not board_dir then
        raise("board dir not found: " .. board_name)
    end
    includes(board_dir)
end

--- @target tar_bsp
--- @brief BSP 聚合静态库
--- @details 依赖选中板级目标 tar_board。
target("tar_bsp")
    set_kind("static")
    add_deps("tar_board", {public = true})
target_end()
