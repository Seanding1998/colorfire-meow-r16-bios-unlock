# SPDX-License-Identifier: MIT
param([switch]$RunTests)
$ErrorActionPreference = 'Stop'
$buildRoot = $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installation) { throw 'Microsoft Visual C++ x64 build tools not found.' }
$versions = Get-ChildItem -LiteralPath (Join-Path $installation 'VC\Tools\MSVC') -Directory | Sort-Object Name -Descending
$bin = Join-Path $versions[0].FullName 'bin\Hostx64\x64'
$compiler = Join-Path $bin 'cl.exe'
$linker = Join-Path $bin 'link.exe'
$build = Join-Path $buildRoot 'build'
New-Item -ItemType Directory -Force -Path $build | Out-Null
foreach ($unit in @('menu_core', 'meowmenu')) {
    & $compiler /nologo /c /TC /GS- /Zl /Oi /Os /W4 /WX /I (Join-Path $buildRoot 'include') "/Fo:$build\$unit.obj" (Join-Path $buildRoot "$unit.c")
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $unit" }
}
& $linker /nologo /nodefaultlib /subsystem:efi_application /entry:EfiMain /machine:x64 /dynamicbase:no /fixed:no /Brepro "/out:$build\MeowMenu.efi" "$build\meowmenu.obj" "$build\menu_core.obj"
if ($LASTEXITCODE -ne 0) { throw 'EFI link failed.' }
Get-FileHash -LiteralPath "$build\MeowMenu.efi" -Algorithm SHA256
if ($RunTests) {
    & $linker /nologo /dll /noentry /nodefaultlib /machine:x64 /export:MenuScan /export:MenuPatchSetup /export:MenuCopyForms /export:MenuCommandMode "/out:$buildRoot\menu_core_test.dll" "$build\menu_core.obj"
    if ($LASTEXITCODE -ne 0) { throw 'Parser test DLL link failed.' }
    & $compiler /nologo /c /TC /GS- /Zl /Oi /Os /W4 /WX /I (Join-Path $buildRoot 'include') "/Fo:$build\test_integration.obj" (Join-Path $buildRoot 'test_integration.c')
    if ($LASTEXITCODE -ne 0) { throw 'Integration test compile failed.' }
    & $linker /nologo /dll /noentry /nodefaultlib /machine:x64 "/out:$buildRoot\integration_test.dll" "$build\test_integration.obj" "$build\menu_core.obj"
    if ($LASTEXITCODE -ne 0) { throw 'Integration test DLL link failed.' }
    & python (Join-Path $buildRoot 'generate_test_fixtures.py')
    if ($LASTEXITCODE -ne 0) { throw 'Synthetic fixture generation failed.' }
    & python (Join-Path $buildRoot 'test_parser.py')
    if ($LASTEXITCODE -ne 0) { throw 'Parser tests failed.' }
    & python (Join-Path $buildRoot 'test_patch.py')
    if ($LASTEXITCODE -ne 0) { throw 'Patch tests failed.' }
    & python (Join-Path $buildRoot 'test_cli.py')
    if ($LASTEXITCODE -ne 0) { throw 'CLI tests failed.' }
    & python (Join-Path $buildRoot 'test_integration.py')
    if ($LASTEXITCODE -ne 0) { throw 'Integration tests failed.' }
}
