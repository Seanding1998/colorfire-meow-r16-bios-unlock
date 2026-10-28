# RD MODE 解锁状态与 Windows BIOS 更新

核查日期：2026-10-28。此说明区分本机已观察到的事实、Windows 官方机制，以及尚未验证的厂商实现。

## 结论

**仅保持 RD MODE=1，本身没有证据说明会必然阻止官方更新或导致变砖。**本项目改的是厂商 Setup 变量的一字节显示开关，没有刷入修改版 BIOS 主镜像，也没有永久安装解锁驱动。

这是基于本项目修改范围和标准更新机制的工程判断，**不是对七彩虹未来更新包的兼容性认证**。我们没有分析或上机测试下一版 COLORFIRE Capsule，不能承诺所有后续版本均无问题，也不能预判更新会保留、清除还是迁移 RD MODE。

## 本机当前证据

本轮只读查询确认：

- 机型：COLORFIRE MEOW R16 Ultra。
- 当前 BIOS：COLORFIRE 1.09。
- 固件设备：COLORFIRE System Firmware 1.09，状态 OK。
- EC 固件设备：COLORFIRE System Firmware EC 1.06，状态 OK。
- 已安装系统固件驱动：oem18.inf，提供者 COLORFIRE Corporation.。
- oem18.inf 的固件资源为 `UEFI\RES_{B497835B-AA21-41C1-9966-2ADF63D36821}`，载荷名称 `N16QF1_109.rom`。
- EC 驱动 oem4.inf 指向独立资源及 `N16QF1_EC_01_06.cap`。

这些数据支持“本机使用 Windows 固件驱动包渠道”，不代表当前已经排队安装新版本，也不能证明未来所有更新只来自该渠道。

本机该次查询没有看到 `Control\FirmwareResources` 注册表键，但已经独立查到两个 UEFI 固件资源设备；不能据一个注册表路径不存在就判定不支持更新。

## Windows 更新机制说明

微软说明：系统固件可以作为驱动包分发，Windows 使用平台信息匹配目标；安装包后，下次启动由 OS Loader 校验载荷目录签名/哈希，再通过 UEFI UpdateCapsule 交给平台固件完成。实际写入和结果处理由平台固件负责。[Windows UEFI 固件更新平台](https://learn.microsoft.com/en-us/windows-hardware/drivers/bringup/windows-uefi-firmware-update-platform)、[安装更新的流程](https://learn.microsoft.com/en-us/windows-hardware/drivers/bringup/installing-the-update)

这条机制不要求用户事先用本工具打开通用写保护。本工具写 Setup 的临时窗口已经结束，最终默认锁1命令在成功日志中返回0；私有锁本身没有读取接口，所以该结果不应被表述为独立读回锁状态。

## 为什么仍有兼容性边界

1. **RD MODE 不只是一个外部菜单补丁文件。**它是被厂商固件读取的 Setup 参数；我们已证明它控制28组隐藏条件，但没有完整证明厂商代码在所有路径上只用它控制显示。因此不能从“主要用途是显示”推导“绝无其他行为”。
2. **更新如何处理 NVRAM 是 OEM 实现问题。**下一版可能保留、迁移或重置参数。当前未取得其更新实现，不能预言具体结果。菜单重新隐藏不等于 BIOS 更新损坏。
3. **用户后来修改的参数是另一组变量。**关闭 RD MODE 不会自动恢复已经保存的内存、电源、PCIe、启动等设置。如果这些设置被改过，应单独记录并评估；不能把 restore.nsh 当作恢复全部原厂配置。
4. **旧解锁工具不是跨版本工具。**当前代码核对 FormSet、VarStore、长度和 RD 问题，但没有按 SMBIOS BIOS 版本进行硬绑定。新的布局或锁处理可能变化，旧检查可能仍然通过，所以升级后不能直接复用。

## 有计划更新时的建议

- 记录当前 BIOS 版本及自己修改过的参数，接通电源，按官方更新提示操作。
- 若希望减少未知配置，在仍运行本项目已分析的1.09时，可以先关闭 RD MODE，并以实际读回0作为确认；这只是保守做法，不是微软或七彩虹已证明必需的前置条件。当前 restore 关闭分支尚未在本机实测，应明确保留这一限制。
- 如果使用 BitLocker/设备加密，确认恢复密钥可取得，并按对应更新机制处理。微软区分 Windows 更新与非微软更新，不应把“所有官方更新都必须手动暂停 BitLocker”作为统一规则。[微软 BitLocker FAQ](https://learn.microsoft.com/en-us/windows/security/operating-system-security/data-protection/bitlocker/faq)
- 更新完成后，先检查 BIOS 版本和系统正常启动，再评估菜单。新 BIOS 未重新分析前，不运行旧版 unlock/restore，不整包写回旧 Setup。

## 自动更新时如何看待

不需要仅因为当前 RD=1 就认定一次自动官方更新一定危险。若已进入固件刷写阶段，应按提示保持供电，让它完成，不要为了提前还原 RD 而强制关机。

自动更新后隐藏菜单若仍在，可先正常使用；若消失，不急着运行旧脚本。先确认新版是否保持相同变量定义及锁接口。更新也可能引入新的最低可回滚版本，不能把手头旧固件备份视为一定可直接降级。[微软固件更新状态与回滚边界](https://learn.microsoft.com/en-us/windows-hardware/drivers/bringup/firmware-update-status)

本次没有安装固件更新、没有变更 Windows 更新策略，也没有修改 BIOS 或 BitLocker 配置。
