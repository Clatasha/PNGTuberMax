param([string]$BuildDir = 'build', [string]$OutputDir = 'dist\PNGTuberMax')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
$exe = Get-ChildItem $BuildDir -Recurse -Filter PNGTuberMax.exe | Where-Object { $_.FullName -match 'Release' } | Select-Object -First 1
if (!$exe) { throw 'Build the Release application first.' }
Copy-Item $exe.FullName $OutputDir -Force
Copy-Item (Join-Path $root 'RahiTuber\res') $OutputDir -Recurse -Force
foreach ($file in @('buildnumber.txt','LICENSE.txt')) { Copy-Item (Join-Path $root $file) $OutputDir -Force }
Copy-Item "$root\RahiTuber\config.xml" $OutputDir -Force
& "$root\camera\Build-Camera.ps1" -OutputDir (Join-Path $root $OutputDir) -BuildDir (Join-Path $root $BuildDir)
foreach ($file in @('Install-Camera.ps1','Uninstall-Camera.ps1')) { Copy-Item "$root\camera\$file" $OutputDir -Force }
New-Item -ItemType Directory "$OutputDir\notices" -Force | Out-Null
Copy-Item "$root\camera\third_party\LICENSE.txt" "$OutputDir\notices\DirectShowLib-LGPL-2.1.txt" -Force
Copy-Item "$root\camera\third_party\DirectShowLib-source.zip" "$OutputDir\notices" -Force
Copy-Item "$root\camera\native\Microsoft-MIT.txt" "$OutputDir\notices\Microsoft-DirectShow-MIT.txt" -Force
foreach ($library in @('SFML','imgui','freetype','mongoose','portaudio','tinyxml2','Spout2','Simple-FFT')) {
    $license = Get-ChildItem "$root\Libraries\$library" -File | Where-Object { $_.Name -match '^(LICENSE|COPYING|License)' } | Select-Object -First 1
    if ($license) { Copy-Item $license.FullName "$OutputDir\notices\$library-license.txt" -Force }
}
Copy-Item "$root\RahiTuber\imgui-sfml\LICENSE" "$OutputDir\notices\imgui-sfml-license.txt" -Force
Copy-Item "$root\docs\RIGGING.md" "$OutputDir\RIGGING.md" -Force
Copy-Item "$root\docs\PREVIEW.txt" "$OutputDir\README.txt" -Force
# Bundle the MSVC runtime installed with the build tools, rather than requiring another installer.
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$crt = Get-ChildItem "$vs\VC\Redist\MSVC" -Directory | Sort-Object Name -Descending | ForEach-Object { Join-Path $_.FullName 'x64\Microsoft.VC143.CRT' } | Where-Object { Test-Path $_ } | Select-Object -First 1
if (!$crt) { throw 'MSVC runtime directory was not found.' }
Copy-Item "$crt\*.dll" $OutputDir -Force
