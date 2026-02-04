# AWLF XMake 新手开发者手册

## 1. 适用范围
本手册面向首次使用 AWLF 的开发者，目标是让你在最短时间内完成可运行的构建，并理解最常用的配置入口。内容完全基于当前仓库代码实现。

## 2. 术语与约定
- board：板级名称，对应 `awlf/platform/bsp/data/boards/<board>.lua`。
- os：OS/RTOS 名称，对应 `awlf/platform/osal/<os>/` 目录。
- toolchain：工具链名称，来自 `awlf/build/toolchains/data.lua`。
- arch traits：芯片提供的架构数据（CPU/FPU/float-abi），由工具链映射为编译参数。
- build profile：输出目录结构，由 XMake `plat/arch/mode` 决定，形如 `build/<plat>/<arch>/<mode>`。

## 3. 环境准备
1) 安装 XMake，并验证：
```sh
xmake --version
```
2) 准备交叉编译工具链（例如 gnu-rm 或 armclang）。
3) 确认工具链路径：
- `sdk`：工具链根目录
- `bin`：可执行文件目录（包含 `arm-none-eabi-gcc` 或 `armclang` 等）

## 4. 项目结构速览
- `xmake.lua`：项目顶层构建入口。
- `awlf/xmake.lua`：AWLF 构建入口转发（实际入口在 `awlf/build/xmake.lua`）。
- `awlf/build/xmake.lua`：AWLF 构建入口。
- `awlf/build/config/`：构建选项定义与默认值。
- `awlf/build/rules/`：构建规则（上下文注入/板级资源/镜像转换）。
- `awlf/build/toolchains/`：工具链数据与脚本逻辑。
- `awlf/build/tasks/`：自定义任务（如 flash 烧录）。
- `awlf/platform/bsp/`：BSP 数据与板级构建脚本。
- `awlf/platform/osal/`：OS 抽象层与 OS 端口。
- `awlf/platform/sync/`：同步原语与加速后端。

## 5. 第一次构建（完整示例）
### 5.1 配置
```sh
xmake f -c --board=rm-c-board --os=freertos --toolchain=gnu-rm \
  --sdk="D:/Toolchains/gcc-arm-none-eabi" --bin="D:/Toolchains/gcc-arm-none-eabi/bin" \
  -m debug
```
说明：
- `-c`：清理旧配置并重新生成。
- `-m debug|release`：构建模式。
- `--board/--os/--toolchain`：必须指定。
- `--sdk/--bin`：必须提供（除非已在 `awlf_preset.lua` 中配置）。
- 已启用 `plugin.compile_commands.autoupdate`，构建后会自动生成 `compile_commands.json` 到项目根目录。

### 5.2 构建
```sh
xmake
```
说明：
- 首次构建会触发工具链检测与配置校验；若工具链不可用会在构建阶段报错。

## 6. 根 `xmake.lua` 最小示例
```lua
set_project("awlf_app")
add_rules("mode.debug", "mode.release")

includes("awlf")

target("app")
    set_kind("binary")
    add_deps("tar_awlf")
    add_rules("awlf.context", "awlf.board_assets", "awlf.image_convert")
    add_files("src/main.c")
    set_policy("check.auto_ignore_flags", false)
target_end()
```

## 7. 构建选项与默认值
### 7.1 选项说明
| 选项 | 说明 | 来源/可选值 |
| --- | --- | --- |
| `--board` | 板级名称 | `awlf/platform/bsp/data/boards/*.lua` |
| `--os` | OS/RTOS 名称 | `awlf/platform/osal/*` |
| `--sync_accel` | 同步加速模式 | `auto` / `none` |
| `--toolchain` | 工具链名称 | `awlf/build/toolchains/data.lua` |
| `--sdk` | 工具链 SDK 目录 | 命令行或 preset 提供 |
| `--bin` | 工具链可执行目录 | 命令行或 preset 提供 |
| `-m` | 构建模式 | `debug` / `release` |

### 7.2 默认值优先级
- board / os：命令行 > `awlf_preset.lua` > `awlf/build/config/defaults.lua` > 数据目录按名称排序的第一个。
- sync_accel：命令行 > `awlf/build/config/defaults.lua`（未配置时等价于 `auto`）。
- toolchain：命令行 > `awlf_preset.lua` 的 `toolchain_default.name` > 已保存配置 > `awlf/build/toolchains/data.lua` 中的 `default`。
- sdk / bin：命令行 > `awlf_preset.lua` 的 `toolchain_presets[toolchain]` > 已保存配置 > toolchain 数据默认值（若都没有会报错）。

## 8. `awlf_preset.lua`（推荐配置方式）
位置：项目根目录（与根 `xmake.lua` 同级）。

用途：提供本机的工具链路径和默认 board/os 选择，避免在命令行重复输入。

示例：
```lua
local preset = {
  toolchain_default = {
    name = "gnu-rm",
  },
  toolchain_presets = {
    ["gnu-rm"] = {
      sdk = "D:/Toolchains/gcc-arm-none-eabi",
      bin = "D:/Toolchains/gcc-arm-none-eabi/bin",
    },
    ["armclang"] = {
      sdk = "D:/Program Files/ProgramTools/Keil_v5/ARM/ARMCLANG",
      bin = "D:/Program Files/ProgramTools/Keil_v5/ARM/ARMCLANG/bin",
    },
  },
  board = {name = "rm-c-board"},
  os = {name = "freertos"},
  flash = {
    jlink = {
      device = "STM32F407IG",
      interface = "swd",
      speed = 4000,
      program = "D:/Program Files/ProgramTools/SEGGER/Jlink/JLink.exe",
      target = "robot_project",
      firmware = nil,
      prefer_hex = true,
      reset = true,
      run = true,
    },
  },
}

function get_preset()
  return preset
end
```
约束：
- 仅用于选择与路径配置，不用于新增 board/os/toolchain 数据。
- `toolchain_default.name` 和 `toolchain_presets` 的键必须在内置数据中存在。
- `flash.jlink` 仅在 `xmake flash` 任务中读取，不影响常规构建流程。

## 9. 构建产物与清理
- 产物路径：`build/<plat>/<arch>/<mode>/`。
- `arch` 为平台架构标识：优先取工具链数据中的 `arch`，若工具链未提供则使用板级 CPU 架构（例如 `cortex-m4`）。  
  因此 armclang 的输出路径可能为 `build/cross/cortex-m4/<mode>/`，属于正常行为。
- 若目标启用 `awlf.image_convert` 规则，会生成 `.hex`/`.bin`。
- 清理：
```sh
xmake clean
```

## 10. 常见问题与排查
- **context missing**：未完成 `xmake f` 或配置不完整。重新执行配置命令。
- **toolchain path not set / sdk not found / bin not found**：未提供 `--sdk/--bin` 或 preset 路径错误。
- **board/os not found**：`--board` 或 `--os` 不存在于数据/目录中。
- **startup/linkerscript not found**：板级/芯片/厂商数据中缺少映射。
- **image tool not found**：工具链镜像转换工具未安装或 `bin` 路径不正确。
- **FreeRTOSConfig.h not found**：板级 OS 配置路径缺失或不正确。
- **FreeRTOS 内存布局**：默认不将 FreeRTOS 对象固定到 `RW_IRAM2`，如需使用 CCM，请调整板级链接脚本。

## 11. 新手常用调整
- 烧录：`xmake flash` 通过 J-Link Commander 烧录，默认优先使用 HEX，可通过 `--firmware` 指定 ELF/HEX。
- 更换板级/OS/工具链：重新执行 `xmake f -c --board=... --os=... --toolchain=...`。
- 关闭镜像生成：从目标规则中移除 `awlf.image_convert`。
- 禁用同步加速：`xmake f -c --sync_accel=none ...`。

## 12. 任务文档
- `awlf/document/build/BuildTasksManual.md`：内置任务与参数说明（含 `xmake flash`）。

## 13. 工程实践
- `awlf/document/build/BuildSystemBestPractices.md`：构建系统最佳工程实践。
