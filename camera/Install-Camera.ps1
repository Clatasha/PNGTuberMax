$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
try {
    $target = Join-Path $env:ProgramFiles 'Clatasha\PNGTuberMaxCamera'
    New-Item -ItemType Directory -Path $target -Force | Out-Null
    foreach ($name in @('PNGTuberMax.Camera.dll','DirectShowLib.dll')) { Copy-Item (Join-Path $PSScriptRoot $name) $target -Force }
    $clsid = '{EEBEE603-EC24-4D77-8827-C8F799C81026}'
    $category = '{860BB310-5D01-11D0-BD3B-00A0C911CE86}'
    foreach ($arch in @('Framework64','Framework')) {
        $regasm = Join-Path $env:WINDIR "Microsoft.NET\$arch\v4.0.30319\RegAsm.exe"
        if (!(Test-Path $regasm)) { continue }
        $registrationLog = & $regasm (Join-Path $target 'PNGTuberMax.Camera.dll') /codebase /nologo 2>&1 | Out-String
        if ($LASTEXITCODE -ne 0) { throw "Camera registration failed ($arch)." }
        $view = if ($arch -eq 'Framework64') {[Microsoft.Win32.RegistryView]::Registry64} else {[Microsoft.Win32.RegistryView]::Registry32}
        $registry = [Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::LocalMachine,$view)
        $key = $registry.CreateSubKey("SOFTWARE\Classes\CLSID\$category\Instance\$clsid")
        $key.SetValue('FriendlyName','PNGTuber Max Camera'); $key.SetValue('CLSID',$clsid)
        $key.Dispose(); $registry.Dispose()
    }
    [System.Windows.Forms.MessageBox]::Show("PNGTuber Max Camera is installed.`r`n`r`nRestart the receiving app and select PNGTuber Max Camera. Click Start virtual camera in PNGTuber Max.",'PNGTuber Max by Clatasha') | Out-Null
} catch {
    [System.Windows.Forms.MessageBox]::Show($_.Exception.Message,'PNGTuber Max camera setup failed') | Out-Null
    exit 1
}
