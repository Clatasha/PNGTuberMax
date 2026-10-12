param([string]$OutputDir = '', [string]$BuildDir = 'build')
$ErrorActionPreference = 'Stop'
if (!$OutputDir) { $OutputDir = Join-Path $PSScriptRoot '..\build\camera' }
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
foreach ($arch in @('x64','x86')) {
    $source=Join-Path $BuildDir "camera-$arch\Release\PNGTuberMax.Camera.dll"
    if (!(Test-Path $source)) { throw "Native camera build is missing ($arch)." }
    $target=if ($arch -eq 'x64') {$OutputDir} else {Join-Path $OutputDir 'camera\x86'}
    New-Item -ItemType Directory -Force -Path $target | Out-Null
    Copy-Item $source $target -Force
}
# This standalone checker uses .NET; the camera loaded into receiving apps is native.
$csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
Copy-Item "$PSScriptRoot\third_party\DirectShowLib.dll" $OutputDir -Force
& $csc /nologo /target:winexe /platform:x64 "/out:$OutputDir\CameraDiagnostic.exe" /reference:System.Drawing.dll /reference:System.Windows.Forms.dll "/reference:$PSScriptRoot\third_party\DirectShowLib.dll" "$PSScriptRoot\CameraDiagnostic.cs" "$PSScriptRoot\CameraInterop.cs"
if ($LASTEXITCODE -ne 0) { throw 'Camera diagnostic compilation failed.' }
