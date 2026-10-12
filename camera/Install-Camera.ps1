param([string]$ResultPath='', [switch]$Quiet)
$ErrorActionPreference='Stop'
$reportDir=Join-Path $env:LOCALAPPDATA 'Clatasha\PNGTuberMax'
New-Item -ItemType Directory $reportDir -Force | Out-Null
$logPath=Join-Path $reportDir 'camera-install-log.txt'
try {
    $root=Join-Path $env:ProgramFiles 'Clatasha\PNGTuberMaxCamera'
    $sources=@{x64=(Join-Path $PSScriptRoot 'PNGTuberMax.Camera.dll');x86=(Join-Path $PSScriptRoot 'camera\x86\PNGTuberMax.Camera.dll')}
    foreach ($file in $sources.Values) { if (!(Test-Path -LiteralPath $file)) { throw 'Extract the complete preview folder, including camera/x86.' } }
    $buildHash=(Get-FileHash -LiteralPath $sources.x64).Hash.Substring(0,12)+(Get-FileHash -LiteralPath $sources.x86).Hash.Substring(0,12)
    $target=Join-Path $root ("native-versions\"+$buildHash)
    Set-Content -LiteralPath $logPath -Value ("Native camera 0.1.7 setup: "+(Get-Date -Format o)+"`r`nTarget: "+$target)
    $clsid='{EEBEE603-EC24-4D77-8827-C8F799C81026}'
    $category='{860BB310-5D01-11D0-BD3B-00A0C911CE86}'
    foreach ($arch in @('x64','x86')) {
        $directory=Join-Path $target $arch
        New-Item -ItemType Directory -Force $directory | Out-Null
        $destination=Join-Path $directory 'PNGTuberMax.Camera.dll'
        $same=(Test-Path -LiteralPath $destination) -and ((Get-FileHash $sources[$arch]).Hash -eq (Get-FileHash $destination).Hash)
        if (!$same) { Copy-Item -LiteralPath $sources[$arch] -Destination $destination -Force }
        Unblock-File -LiteralPath $destination
        $view=if ($arch -eq 'x64') {[Microsoft.Win32.RegistryView]::Registry64} else {[Microsoft.Win32.RegistryView]::Registry32}
        $registry=[Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::LocalMachine,$view)
        # Remove the old CLR loader registration and its version-specific values.
        $registry.DeleteSubKeyTree("SOFTWARE\Classes\CLSID\$clsid\InprocServer32",$false)
        $server=$registry.CreateSubKey("SOFTWARE\Classes\CLSID\$clsid\InprocServer32")
        $server.SetValue('',$destination);$server.SetValue('ThreadingModel','Both');$server.Dispose()
        $key=$registry.CreateSubKey("SOFTWARE\Classes\CLSID\$category\Instance\$clsid")
        $key.SetValue('FriendlyName','PNGTuber Max Camera');$key.SetValue('CLSID',$clsid);$key.Dispose()
        $legacyPath="SOFTWARE\Classes\CLSID\$category\Instance\{B9707C62-8759-4988-9885-1239742EF331}"
        $legacy=$registry.OpenSubKey($legacyPath);$retire=$legacy -and ($legacy.GetValue('FriendlyName') -eq 'Clatasha Camera (Preview)')
        if ($legacy) { $legacy.Dispose() }
        if ($retire) { $registry.DeleteSubKeyTree($legacyPath,$false) }
        $registry.Dispose()
        Add-Content -LiteralPath $logPath -Value ("Native "+$arch+": "+$destination+"`r`nSHA256: "+(Get-FileHash $destination).Hash)
    }
    $message='SUCCESS: Native camera 0.1.7 installed (32-bit and 64-bit). Fully exit and reopen OBS / Mirillis Action to unload the previous managed camera.'
    $exitCode=0
} catch { $message='ERROR: '+$_.Exception.Message+"`r`nDetails: $logPath"; $exitCode=1 }
if ($ResultPath) { [System.IO.File]::WriteAllText($ResultPath,$message) }
if (!$Quiet -and !$ResultPath) { Add-Type -AssemblyName System.Windows.Forms;[System.Windows.Forms.MessageBox]::Show($message,'PNGTuber Max camera setup') | Out-Null }
exit $exitCode
