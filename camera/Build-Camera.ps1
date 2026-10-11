param([string]$OutputDir = '')
$ErrorActionPreference = 'Stop'
if (!$OutputDir) { $OutputDir = Join-Path $PSScriptRoot '..\build\camera' }
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
if (!(Test-Path $csc)) { throw '.NET Framework 4.8 compiler is required.' }
& $csc /nologo /target:library /platform:anycpu "/out:$OutputDir\PNGTuberMax.Camera.dll" /reference:System.Drawing.dll "/reference:$PSScriptRoot\third_party\DirectShowLib.dll" "$PSScriptRoot\VirtualCamera.cs"
if ($LASTEXITCODE -ne 0) { throw 'Camera compilation failed.' }
Copy-Item "$PSScriptRoot\third_party\DirectShowLib.dll" $OutputDir -Force
