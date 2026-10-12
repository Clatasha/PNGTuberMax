param([string]$ResultPath='', [switch]$Quiet)
$ErrorActionPreference='Stop'
$reportDir=Join-Path $env:LOCALAPPDATA 'Clatasha\PNGTuberMax'
New-Item -ItemType Directory $reportDir -Force | Out-Null
$logPath=Join-Path $reportDir 'camera-install-log.txt'
try {
    $root=Join-Path $env:ProgramFiles 'Clatasha\PNGTuberMaxCamera'
    $cameraSource=Join-Path $PSScriptRoot 'PNGTuberMax.Camera.dll'
    if (!(Test-Path -LiteralPath $cameraSource)) { throw 'Extract the whole preview folder before camera setup.' }
    $buildHash=(Get-FileHash -LiteralPath $cameraSource -Algorithm SHA256).Hash
    # Keep loaded old DLLs untouched. New registrations use a new immutable build directory.
    $target=Join-Path $root ("versions\"+$buildHash.Substring(0,16))
    Set-Content -LiteralPath $logPath -Value ("Camera setup: "+(Get-Date -Format o)+"`r`nBuild SHA256: "+$buildHash+"`r`nTarget: "+$target)
    New-Item -ItemType Directory $target -Force | Out-Null
    foreach ($name in @('PNGTuberMax.Camera.dll','DirectShowLib.dll')) {
        $source=Join-Path $PSScriptRoot $name
        if (!(Test-Path -LiteralPath $source)) { throw "Missing $name. Extract the whole preview folder before camera setup." }
        $destination=Join-Path $target $name
        $same=(Test-Path -LiteralPath $destination) -and ((Get-FileHash -LiteralPath $source).Hash -eq (Get-FileHash -LiteralPath $destination).Hash)
        if (!$same) { Copy-Item -LiteralPath $source -Destination $destination -Force }
        # Windows marks DLLs extracted from internet ZIPs. RegAsm otherwise refuses to load them.
        Unblock-File -LiteralPath $destination
    }
    $clsid='{EEBEE603-EC24-4D77-8827-C8F799C81026}'
    $category='{860BB310-5D01-11D0-BD3B-00A0C911CE86}'
    $registered=0
    foreach ($arch in @('Framework64','Framework')) {
        $regasm=Join-Path $env:WINDIR "Microsoft.NET\$arch\v4.0.30319\RegAsm.exe"
        if (!(Test-Path -LiteralPath $regasm)) { continue }
        $stdout=Join-Path $reportDir "registration-$arch-out.txt"
        $stderr=Join-Path $reportDir "registration-$arch-error.txt"
        $process=Start-Process -FilePath $regasm -ArgumentList ('"'+(Join-Path $target 'PNGTuberMax.Camera.dll')+'" /codebase /nologo') -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
        $details=(Get-Content $stdout -Raw -ErrorAction SilentlyContinue)+(Get-Content $stderr -Raw -ErrorAction SilentlyContinue)
        Add-Content -LiteralPath $logPath -Value "$arch`r`n$details"
        if ($process.ExitCode -ne 0) { throw "Registration failed ($arch): $details" }
        $view=if ($arch -eq 'Framework64') {[Microsoft.Win32.RegistryView]::Registry64} else {[Microsoft.Win32.RegistryView]::Registry32}
        $registry=[Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::LocalMachine,$view)
        $key=$registry.CreateSubKey("SOFTWARE\Classes\CLSID\$category\Instance\$clsid")
        $key.SetValue('FriendlyName','PNGTuber Max Camera');$key.SetValue('CLSID',$clsid)
        $key.Dispose()
        # Retire only our first prototype's device listing; keep its COM class and files intact.
        $legacyPath="SOFTWARE\Classes\CLSID\$category\Instance\{B9707C62-8759-4988-9885-1239742EF331}"
        $legacyKey=$registry.OpenSubKey($legacyPath)
        $retire=$legacyKey -and ($legacyKey.GetValue('FriendlyName') -eq 'Clatasha Camera (Preview)')
        if ($legacyKey) { $legacyKey.Dispose() }
        if ($retire) { $registry.DeleteSubKeyTree($legacyPath,$false); Add-Content -LiteralPath $logPath -Value "Retired obsolete Clatasha Camera (Preview) device entry ($arch)." }
        $registry.Dispose();$registered++
    }
    if ($registered -eq 0) { throw '.NET Framework registration tools were not found.' }
    $message='SUCCESS: Camera build installed. Fully close and reopen OBS / Mirillis Action before selecting PNGTuber Max Camera. Existing receiver processes may still hold the previous DLL.'
    $exitCode=0
} catch {
    $message='ERROR: '+$_.Exception.Message+"`r`nDetails: $logPath"
    $exitCode=1
}
if ($ResultPath) { [System.IO.File]::WriteAllText($ResultPath,$message) }
if (!$Quiet -and !$ResultPath) { Add-Type -AssemblyName System.Windows.Forms;[System.Windows.Forms.MessageBox]::Show($message,'PNGTuber Max camera setup') | Out-Null }
exit $exitCode
