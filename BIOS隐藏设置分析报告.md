# BIOS 隐藏菜单工程：当前有效结论

更新日期：2026-10-28；当前交付版本：MEOW Menu 0.5。

目标设备为 COLORFIRE MEOW R16 Ultra / N16QF1 / AMI Aptio V 1.09。旧版完整研究报告已保存在历史归档，此文件替换其中已失效的操作建议，原始分析数据仍保留。

## 已获得的运行证据

1. 用户通过固件原生 FormBrowser2 进入独立 CBS 和 PBS FormSet，能查看原生底层页面。
2. 对当前 HII Setup Forms 内 28 个 RD MODE 条件做长度不变的比较常数替换后，用户能进入先前隐藏的 Setup 菜单；退出恢复与回读均返回成功。
3. 仅在原生界面修改 RD MODE、F10 保存没有产生用户预期的重启后效果。旧日志没有读取持久变量，因此不能只凭 SendForm 成功判断配置已保存。
4. 0.3 实测日志：Setup 长 255 字节、属性 3、原 Setup[0x8A]=0；直接 SetVariable 返回 SECURITY_VIOLATION；受控窗口内写入成功，最终 level 1 命令成功，完整变量回读通过且目标值为 1。
5. 用户随后确认重启进入普通 BIOS 已出现隐藏设置，证明该机器的 RD 开启已跨启动生效。

## 当前控制项

`Setup` GUID 为 `ec87d643-eba4-4bb5-a1e5-3f3e36b20da9`，变量长度 255 字节。RD MODE Support 的 QuestionId 为 `0xA6`，VarStoreId 为 `0xF101`，偏移 `0x8A`，值 0=Disabled、1=Enabled。

Setup FormSet 为 `7b59104a-c00d-4158-87ff-f04d6396a915`，Advanced FormId 为 `0x2712`。

独立 CBS 为 `c03209ed-5f4b-4631-bd03-8ed627cd9daf / 0x7000`，PBS 为 `b863b959-0ec6-4033-99c1-8fd89f040222 / 0x000B`。通过原生浏览器按这些目标直接访问，绕过的是 Setup 中隐藏的入口，不是解除所有变量权限或所有平台条件。

## 持久写入机制

只使用当前实时 Setup 的副本，改 `0x8A` 目标字节，并保留原属性及其他内容。只接受长度 255、属性 3/7、旧值 0/1 和匹配的本机 HII 身份。写前落盘备份，写后回读整个变量与属性。

本机二进制验证的私有邮箱为 `NvLockMailbox`，GUID `504AF431-3025-4D32-9FBF-E9F31855551D`，BS 属性 2，6 字节数据 `ANV$ + level + 0`。它是 SMM 命令，不应把它当成可读取的持久变量。

只有直接写入返回 SECURITY_VIOLATION，且变量未变化、当前 Setup[0x55]==1 时，才进入受控窗口：主动设默认 level 1、临时 level 0、执行唯一目标写入、最终强制 level 1。第一次邮箱操作后的正常错误路径也执行收尾。原始私有锁状态不可读，不能声称恢复了读取到的旧状态。

## 一键还原的准确语义

0.5 的 `restore.nsh` 调用 `--disable-rd`，目标值为 0。还原前再次备份当前变量为 `MeowSetupBeforeRestoreNNN.bin`；不读取旧完整 Setup 来覆盖当前值。已为 0 时无变量和邮箱写入。

还原只关闭 RD MODE 显示开关，不重置 CBS/PBS 已保存参数，不处理启动项，不改 FAR、SPL，不恢复出厂设置，也不刷写 BIOS 主镜像。

## 旧方案状态

- 直接将 `12 06` 改为 `47 06` 的 IFR 方案存在指令长度疑点，已经废弃。
- 原 SREP.efi 是 EFI Application，不能把登记 Driver 条目等同于补丁已执行；原启动项已由用户删除，旧程序停用并归档。
- 不再使用旧 UVT/SREP 操作脚本作为正式入口。
- FAR 是固件防回滚控制，不是写入解锁开关；本次工具未修改 FAR 或 SPL。

## 本轮验证边界

0.3 的启用方向和重启后菜单显示已经在本机验证。0.5 参数化的启用、还原及错误路径通过 59 个宿主协议模拟场景，CLI 通过 23 个用例；关闭方向及新封装尚未执行本机写入验证。

原始 SPI dump 位于独立的 Desktop/bios 目录，保持原样，本次工程同步未触及。
