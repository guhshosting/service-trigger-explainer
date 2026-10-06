param([string]$Build='build/debug')
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force artifacts | Out-Null
$log=Join-Path (Get-Location) 'artifacts/clang-tidy.txt'
Set-Content -LiteralPath $log -Value ''
foreach ($file in (Get-ChildItem src,tests -Filter '*.cpp')) {
    "Checking $($file.FullName)" | Tee-Object -FilePath $log -Append
    $result= & 'C:\Program Files\LLVM\bin\clang-tidy.exe' $file.FullName -p $Build --quiet 2>&1
    $result | Tee-Object -FilePath $log -Append
    if ($LASTEXITCODE -ne 0) { throw "clang-tidy failed: $file" }
}
& 'C:\Program Files\Cppcheck\cppcheck.exe' --enable=warning,style,performance,portability --error-exitcode=1 --std=c++17 --platform=win64 --inline-suppr --suppress=missingIncludeSystem -I include src tests 2>&1 | Tee-Object -FilePath artifacts/cppcheck.txt
if ($LASTEXITCODE -ne 0) { throw 'Cppcheck failed' }
'clang-tidy and Cppcheck passed.' | Tee-Object -FilePath $log -Append
