param([Parameter(Mandatory)][ValidateSet('debug','release','asan')][string]$Preset)
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force artifacts | Out-Null
& cmake --preset $Preset 2>&1 | Tee-Object -FilePath "artifacts/$Preset-configure.txt"
if ($LASTEXITCODE -ne 0) { throw 'Configure failed' }
& cmake --build --preset $Preset 2>&1 | Tee-Object -FilePath "artifacts/$Preset-build.txt"
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
& ctest --preset $Preset 2>&1 | Tee-Object -FilePath "artifacts/$Preset-tests.txt"
if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
