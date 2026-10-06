param([string]$App='build/release/service-trigger-explainer.exe')
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force artifacts | Out-Null
$imports=(& dumpbin.exe /imports $App) -join "`n"
if ($LASTEXITCODE -ne 0) { throw 'Import extraction failed' }
Set-Content -LiteralPath artifacts/imports.txt -Value $imports
if ($imports -match '(StartService|ControlService|ChangeServiceConfig|CreateService|DeleteService|WSAStartup|WS2_32|WINHTTP|WININET|RegSetValue|RegCreateKey|CreateProcess|ShellExecute)') { throw 'Forbidden import' }
& mt.exe -nologo "-inputresource:$App;#1" -out:artifacts/application.manifest
if ($LASTEXITCODE -ne 0) { throw 'Manifest extraction failed' }
if ((Get-Content -Raw artifacts/application.manifest) -notmatch 'level="asInvoker"') { throw 'Unexpected elevation manifest' }
'Import audit passed; manifest requests asInvoker. Application imports no service mutation, network, registry-write, or subprocess API.' | Tee-Object -FilePath artifacts/import-audit.txt
