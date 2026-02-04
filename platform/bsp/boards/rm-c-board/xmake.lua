--- @file awlf/platform/bsp/boards/rm-c-board/xmake.lua
--- @brief rm-c-board BSP 构建脚本
--- @details 负责板级源文件与组件资源的编译注入。

--- @target tar_board
--- @brief rm-c-board 板级静态库
--- @details 注入板级构建输入并依赖基础驱动与 API。
target("tar_board")
    set_kind("static")
    add_rules("awlf.context")
    add_deps("tar_awapi_pal", {public = true})
    add_deps("tar_awdrivers", {public = true})
    --- 目标加载阶段注入板级构建输入
    ---@param target target 目标对象
    on_load(function(target)
        -- 解析上下文并加载板级构建输入
        local board_root = os.scriptdir()
        local bsp_root = path.join(board_root, "..", "..")
        local awlf_root = path.join(bsp_root, "..", "..")
        local modules_root = path.join(awlf_root, "build", "modules")
        local awlf = import("awlf", {rootdir = modules_root})
        local context = awlf.get_context()
        local board_name = context.board_name
        local bsp = import("bsp", {rootdir = bsp_root})
        local inputs = bsp.get_board_build_inputs(board_name)
        if inputs.includedirs and #inputs.includedirs > 0 then
            target:add("includedirs", inputs.includedirs, {public = false})
        end
        if inputs.defines and #inputs.defines > 0 then
            target:add("defines", inputs.defines)
        end
        if inputs.sources and #inputs.sources > 0 then
            target:add("files", inputs.sources)
        end
        if inputs.headerfiles and #inputs.headerfiles > 0 then
            target:add("headerfiles", inputs.headerfiles)
        end
        if inputs.extrafiles and #inputs.extrafiles > 0 then
            target:add("extrafiles", inputs.extrafiles)
        end
    end)
target_end()
