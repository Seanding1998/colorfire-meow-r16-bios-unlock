# COLORFIRE MEOW R16 BIOS Unlock

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![UEFI x64](https://img.shields.io/badge/platform-UEFI%20x64-555.svg)
![Target BIOS 1.09](https://img.shields.io/badge/target-N16QF1%20%2F%20AMI%201.09-orange.svg)

**为 COLORFIRE MEOW R16 Ultra 提供原生 CBS/PBS 入口、Setup 隐藏菜单访问，以及 RD MODE 的一键开启与还原。**

Native UEFI form access and RD MODE tooling for the COLORFIRE MEOW R16 Ultra. This is a model-specific firmware configuration tool, not a generic BIOS flasher.

本项目从实际固件结构分析出发，使用固件现有的表单浏览器访问菜单；持久开启只修改当前 `Setup` 变量的 `0x8A` 字节。**已在一台目标机器上确认：写入 RD MODE 后，重启进入普通 BIOS，隐藏设置仍然出现。**

> **适用范围：N16QF1、AMI Aptio V BIOS 1.09、x64 UEFI。** 这不是跨机型、跨 BIOS 版本的通用解锁工具。新版 BIOS 未重新分析前，不应直接运行旧版开启或还原脚本。

## 功能与验证状态

| 功能 | 行为 | 验证情况 |
|---|---|---|
| 原生 AMD CBS / PBS | 调用现有 `FormBrowser2.SendForm` 打开独立 FormSet | 目标机器已验证可进入 |
| 临时 Setup 解锁 | 修改当前 HII Forms 中 28 个 RD MODE 条件；退出尝试恢复 | 目标机器已验证显示及恢复 |
| 持久开启 RD MODE | 将 `Setup[0x8A]` 设为 `1`，完整回读核验 | 0.3 核心流程已验证，普通 BIOS 跨重启显示菜单 |
| 一键还原 | 将同一字节设为 `0`，保留其他当前设置 | 已完成宿主模拟；**关闭方向尚未真机验证** |
| 重复执行 | 当前值已等于目标时，不写 Setup 或锁邮箱 | 宿主模拟通过 |

当前源码版本为 **0.5**。后续一键封装与还原能力，不应被误写成所有分支都已完成真机测试。

## 开始前

- 确认设备、BIOS 版本与本项目目标一致，并保留自己机器的原始备份。
- 隐藏菜单显示不等于所有设置都适用于本机，也不代表任意刷写权限已经解除。
- `restore.nsh` 还原的是 **RD MODE 显示开关**，不是恢复出厂设置，不会撤销已保存的 CBS/PBS 参数。
- 变量操作是真实的持久写入；程序提供备份、身份检查和回读校验，仍不能保证不存在固件实现问题。

## 快速开始

### 1. 获取工程

```powershell
git clone https://github.com/Seanding1998/colorfire-meow-r16-bios-unlock.git
cd colorfire-meow-r16-bios-unlock
```

仓库提供本项目自行编译的 `MeowMenu.efi`、脚本和源码。**不包含厂商 BIOS、真实变量备份或第三方 UEFI Shell 二进制。**

请另行准备可信、许可允许使用的 **x64 UEFI Shell**，放到：

```text
USB_一键工具/EFI/Boot/bootx64.efi
```

上游源码可参考 [TianoCore ShellPkg](https://github.com/tianocore/edk2/tree/master/ShellPkg)。制作脚本会检查 Shell 文件是否存在；缺失时明确退出，不会悄悄下载或替换。

### 2. 同步到 FAT32 U 盘

在 Windows PowerShell 中执行，`E` 换成实际 U 盘盘符：

```powershell
.\制作解锁U盘.ps1 -DriveLetter E
```

**此脚本不会格式化、不会重新分区，也不会执行 BIOS 写入。**它向已准备好的 FAT32 可移动盘复制工具；非 FAT32 会报错退出。同名工具覆盖前先备份，已有 `DATA` / `BACKUP` 文件保留；不同的现有 EFI 启动器不会被覆盖。

### 3. 启动并操作

重启按目标机器的启动菜单键（已验证机器使用 **F7 → UEFI: LUMIX**）。默认启动只进入菜单，不自动开启或关闭 RD MODE。

在工具菜单按 `Q` 返回 Shell，然后在 U 盘根目录运行：

```text
unlock.nsh
```

需要关闭 RD MODE 时：

```text
restore.nsh
```

不在 U 盘根目录时，可使用当次完整路径，例如 `fs4:\unlock.nsh`。**`fs4:` 不是固定编号**；脚本会扫描 `fs0..fs31`，仅在找到唯一工具卷后调用程序。

这些 `.nsh` 是 **UEFI Shell 脚本**，不是 Windows 批处理。工具完成后手动 `reset`，检查普通 BIOS 时不再从 U 盘启动。脚本不会自动重启，也不会无条件打印“解锁成功”。

## 菜单和命令行

| 菜单 | 操作 |
|---|---|
| 1 / 2 | 打开原生 AMD CBS / AMD PBS |
| 3 | 打开未经补丁的 Setup Advanced |
| 4 | 当前会话临时解锁 Setup 隐藏项 |
| 5 | 仅尝试直接写入 RD MODE，不调整锁策略 |
| 6 | 受控开启 RD MODE |
| 7 | 受控关闭 RD MODE |
| Q | 返回 Shell |

EFI 程序同时支持互斥的参数：

```text
MeowMenu.efi --menu
MeowMenu.efi --enable-rd
MeowMenu.efi --disable-rd
```

不带参数默认显示菜单。未知、重复或冲突参数会被拒绝，不落入持久写入分支。

## 实现方式

### 菜单访问

程序从当前 HII 数据库导出表单，通过 **FormSet GUID + 根 FormId** 识别 CBS、PBS、Setup；随后调用固件已安装的原生浏览器。不会为了显示页面重新注入厂商 DXE 或加载另一套 SetupBrowser。

临时解锁只在目标 Setup FormSet 内处理 28 个条件：

```text
SuppressIf (RD_MODE == 0)  →  SuppressIf (RD_MODE == 0xFFFF)
```

RD MODE 是 uint8 问题，比较常数的改变保留了原操作码和长度。程序只提交 Forms 包，更新后回读确认，再显示页面；退出后尝试恢复原 Forms，并核验恢复结果。

### 持久开启与还原

1. 核对 HII 身份、变量长度、属性和 RD 问题定义。
2. 实时读取完整 255 字节 `Setup`；只接受原属性 `3` 或 `7`、RD 值 `0/1`。
3. 当前值已为目标值时直接返回，不写变量或邮箱。
4. 将当前原始变量备份到 `MEOW/DATA`，确认写入长度和 Flush 成功。
5. 复制当前数据，只修改 `0x8A`，保留其他 254 字节和原属性。
6. 优先正常写入。只有被 `SECURITY_VIOLATION` 拒绝、原数据未变化且本机锁策略条件匹配时，才使用受控写入窗口。
7. 使用窗口后，正常错误路径也尝试强制设回本机 DXE 默认锁级别 `1`。
8. 完整回读变量与属性，任何不一致都不报告成功。

私有锁没有读取接口。这里是**主动设回已分析固件的默认策略**，不是恢复一个已读取的旧状态。该机制也是工具不能直接用于其他 BIOS 版本的重要原因。

更多说明见 [架构与边界](docs/ARCHITECTURE.md) 和 [验证记录](docs/VALIDATION.md)。

## 从源码构建与测试

需要 **Windows x64、Visual Studio C++ x64 Build Tools、64 位 Python 3.11+**。EDK II 头文件已按固定提交附带，无需完整 EDK II 工作区，也没有第三方 Python 包依赖。

```powershell
.\源码\build.ps1 -RunTests
```

输出：`源码/build/MeowMenu.efi`。测试会自动生成合成 HII 样本，运行生产解析器和模拟协议；**不读取或写入当前机器的 BIOS 设置**。

公开测试包括 59 个宿主协议场景、23 个 CLI 用例，以及畸形包、重复目标、28 条补丁计数和 12,000 个确定性随机输入。真实固件样本保留在开发者本地，不是构建或测试的必要条件。

构建不会自动替换附带的预编译程序，也不会复制到 U 盘。发布前应单独核对构建产物与校验值。

## 日志、备份与目录

```text
源码/                         C 应用、解析器、构建与测试代码
docs/                         架构、验证和公开发布说明
USB_一键工具/
├─ startup.nsh               默认菜单
├─ menu.nsh                  菜单入口
├─ unlock.nsh                一键开启
├─ restore.nsh               一键关闭 RD MODE
└─ MEOW/
   ├─ MeowMenu.efi           本项目预编译程序
   ├─ USB.TAG                工具卷定位标记
   └─ DATA/                  运行时自动保存日志、快照和操作前备份
```

`DATA`、`BACKUP`、固件镜像、真实 HII 样本、旧工程归档和本机路径清单均不纳入公开 Git 历史。公开仓库与开发者保留完整资料的本地工程可以共存。

## BIOS 更新

本次解锁没有替换 BIOS 主镜像。仅 RD MODE=1 不应被直接解释为官方 Windows 固件更新必然失败；但下一版厂商更新如何迁移配置尚未验证，菜单可能保留或重新隐藏。

**更新 BIOS 后，先不要运行旧版 unlock / restore，也不要整包写回旧 Setup。**目前的结构检查没有按 BIOS 版本硬绑定，不能保证新版本兼容。详见 [BIOS 更新兼容性说明](BIOS更新兼容性说明.md)。

## 参与和问题反馈

提交问题时请提供机型、BIOS 版本、所用工具版本，以及去除敏感内容后的相关错误码。不要把原始 SPI、完整 NVRAM、序列号、MAC 地址或密钥直接上传到公开 Issue。新机型支持需要独立分析与验证，不能只替换几个 GUID 后宣称支持。

## 许可证与声明

项目自有代码和文档采用 [MIT License](LICENSE)。EDK II 头文件保留原有 **BSD-2-Clause-Patent** 等许可和版权声明，见 [第三方说明](THIRD_PARTY_NOTICES.md)。

这是独立研究项目，与 COLORFIRE、AMI、AMD 没有官方合作或背书关系。品牌名称仅用于说明适用设备和互操作背景。
