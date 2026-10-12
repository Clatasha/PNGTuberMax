$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Windows.Forms
try {
 $clsid='{EEBEE603-EC24-4D77-8827-C8F799C81026}';$category='{860BB310-5D01-11D0-BD3B-00A0C911CE86}'
 foreach ($view in @([Microsoft.Win32.RegistryView]::Registry64,[Microsoft.Win32.RegistryView]::Registry32)) {
  $registry=[Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::LocalMachine,$view)
  $registry.DeleteSubKeyTree("SOFTWARE\Classes\CLSID\$category\Instance\$clsid",$false)
  $registry.DeleteSubKeyTree("SOFTWARE\Classes\CLSID\$clsid",$false);$registry.Dispose()
 }
 $target=Join-Path $env:ProgramFiles 'Clatasha\PNGTuberMaxCamera'
 if (Test-Path $target) { Remove-Item $target -Recurse -Force }
 [System.Windows.Forms.MessageBox]::Show('Camera removed. Your saved characters were retained.','PNGTuber Max') | Out-Null
} catch { [System.Windows.Forms.MessageBox]::Show($_.Exception.Message,'Camera removal failed') | Out-Null;exit 1 }
