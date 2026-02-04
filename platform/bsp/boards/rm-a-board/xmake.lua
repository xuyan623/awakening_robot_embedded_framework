--- @file awlf/platform/bsp/boards/rm-a-board/xmake.lua
--- @brief rm-a-board BSP 构建脚本
--- @details 当前未提供 rm-a-board 的完整板级实现。
local board_root = os.scriptdir()
local bsp_root = path.join(board_root, "..", "..")
local awlf_root = path.join(bsp_root, "..", "..")
local modules_root = path.join(awlf_root, "build", "modules")
local awlf = import("awlf", {rootdir = modules_root})
local context = awlf.get_context()

if context.board_name == "rm-a-board" then
    raise("rm-a-board not implemented: missing board data and sources.")
end
