# SPDX-License-Identifier: MIT
<#
【重要操作说明：本脚本不会格式化 U 盘】
1. 不格式化、不重新分区，不删除 U 盘上的其他文件。
2. 只复制/更新工程的同名工具文件；需覆盖的旧文件先备份到
   目标盘:\MEOW\SYNC_BACKUP\时间戳\，并核对备份 SHA256。
3. 已存在的 MEOW\DATA 与 MEOW\BACKUP 文件保留，不被模板覆盖。
4. 目标必须是已准备好的 FAT32 可移动 U 盘；否则报错退出，不自动转换或格式化。
5. 目标已有不同的 EFI 启动器时拒绝覆盖；运行前务必核对 -DriveLetter 盘符。
6. 本脚本只操作 U 盘文件，不运行 EFI，不修改 BIOS/NVRAM。
使用示例：.\制作解锁U盘.ps1 -DriveLetter E
详细步骤请先阅读同目录“使用方法.txt”。
#>
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[D-Zd-z]$')]
    [string]$DriveLetter
)
$ErrorActionPreference = 'Stop'
$sourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'USB_一键工具')).Path.TrimEnd('\')
$targetRoot = $DriveLetter.ToUpper() + ':\'
Write-Host '【重要】本脚本不会格式化或重新分区 U 盘。' -ForegroundColor Yellow
Write-Host ("【核对盘符】目标为 {0}；同名工具文件会先备份再更新，DATA/BACKUP 已有文件保留。" -f $targetRoot) -ForegroundColor Yellow
$device = $DriveLetter.ToUpper() + ':'
$volume = Get-CimInstance Win32_LogicalDisk -Filter "DeviceID='$device'"
if (-not $volume -or $volume.DriveType -ne 2 -or $volume.FileSystem -ne 'FAT32') {
    throw '目标必须是已经准备好的 FAT32 可移动 U 盘。本脚本不会格式化或自动转换文件系统。'
}
if ($env:SystemDrive -eq $device) { throw 'Refusing to write to the Windows system drive.' }
$sourceBoot = Join-Path $sourceRoot 'EFI\Boot\bootx64.efi'
if (-not (Test-Path -LiteralPath $sourceBoot -PathType Leaf)) {
    throw 'Public repository does not include UEFI Shell. Supply a licensed x64 Shell at USB_一键工具\EFI\Boot\bootx64.efi before copying.'
}
$targetBoot = Join-Path $targetRoot 'EFI\Boot\bootx64.efi'
if (Test-Path -LiteralPath $targetBoot) {
    if ((Get-FileHash -LiteralPath $sourceBoot).Hash -ne (Get-FileHash -LiteralPath $targetBoot).Hash) {
        throw 'A different bootloader exists on the target. It will not be overwritten.'
    }
}
$stamp = Get-Date -Format 'yyyyMMdd_HHmmss_fff'
$backupRoot = Join-Path $targetRoot "MEOW\SYNC_BACKUP\$stamp"
$copied = 0
$preserved = 0
foreach ($file in Get-ChildItem -LiteralPath $sourceRoot -File -Recurse) {
    if (($file.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw 'Reparse point in source; stopping.' }
    $relative = $file.FullName.Substring($sourceRoot.Length).TrimStart('\')
    $destination = Join-Path $targetRoot $relative
    $runtimeFile = $relative.StartsWith('MEOW\DATA\', [StringComparison]::OrdinalIgnoreCase) -or $relative.StartsWith('MEOW\BACKUP\', [StringComparison]::OrdinalIgnoreCase)
    if (Test-Path -LiteralPath $destination) {
        if ($runtimeFile) { $preserved++; continue }
        if ((Get-FileHash -LiteralPath $file.FullName).Hash -eq (Get-FileHash -LiteralPath $destination).Hash) { continue }
        $backup = Join-Path $backupRoot $relative
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $backup) | Out-Null
        Copy-Item -LiteralPath $destination -Destination $backup
        if ((Get-FileHash -LiteralPath $backup).Hash -ne (Get-FileHash -LiteralPath $destination).Hash) { throw "Backup verification failed: $relative" }
    }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
    Copy-Item -LiteralPath $file.FullName -Destination $destination -Force
    if ((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath $destination).Hash) { throw "Copy verification failed: $relative" }
    $copied++
}
Write-Host "USB synchronization complete. Copied or updated: $copied; preserved runtime files: $preserved"
Write-Host 'No files were deleted. No formatting, EFI execution or BIOS variable write was performed.'
Write-Host 'UEFI Shell: unlock.nsh enables RD MODE; restore.nsh disables only RD MODE; menu.nsh opens the menu.'
