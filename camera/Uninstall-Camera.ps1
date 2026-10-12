$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
try {
    $target = Join-Path $env:ProgramFiles 'Clatasha\PNGTuberMaxCamera'
    $clsid = '{EEBEE603-EC24-4D77-8827-C8F799C81026}'
    $registeredDll=Join-Path $target 'PNGTuberMax.Camera.dll'
    $registration=Get-ItemProperty -LiteralPath ('Registry::HKEY_CLASSES_ROOT\CLSID\'+$clsid+'\InprocServer32') -ErrorAction SilentlyContinue
    if ($registration.CodeBase) { $registeredDll=([Uri]$registration.CodeBase).LocalPath }
    $category = '{860BB310-5D01-11D0-BD3B-00A0C911CE86}'
    foreach ($arch in @('Framework64','Framework')) {
        $regasm = Join-Path $env:WINDIR "Microsoft.NET\$arch\v4.0.30319\RegAsm.exe"
        if ((Test-Path $regasm) -and (Test-Path -LiteralPath $registeredDll)) { & $regasm $registeredDll /unregister /nologo | Out-Null; if ($LASTEXITCODE -ne 0) { throw "Camera unregistration failed ($arch)." } }
        $view = if ($arch -eq 'Framework64') {[Microsoft.Win32.RegistryView]::Registry64} else {[Microsoft.Win32.RegistryView]::Registry32}
        $registry = [Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::LocalMachine,$view)
        $registry.DeleteSubKeyTree("SOFTWARE\Classes\CLSID\$category\Instance\$clsid",$false);$registry.Dispose()
    }
    if (Test-Path $target) { Remove-Item $target -Recurse -Force }
    [System.Windows.Forms.MessageBox]::Show('The virtual camera was removed. Your saved characters were retained.','PNGTuber Max by Clatasha') | Out-Null
} catch { [System.Windows.Forms.MessageBox]::Show($_.Exception.Message,'Camera removal failed') | Out-Null; exit 1 }
