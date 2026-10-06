param([Parameter(Mandatory)][string]$App, [Parameter(Mandatory)][string]$Tests, [Parameter(Mandatory)][string]$Schema)
$ErrorActionPreference='Stop'
function Validate-Response([string]$text) {
    if (-not (Test-Json -Json $text -SchemaFile $Schema)) { throw 'Schema validation failed' }
    $r=$text | ConvertFrom-Json
    if ($r.PSObject.Properties.Name -contains 'triggers') {
        if ($r.trigger_count -ne $r.triggers.Count) { throw 'Trigger count mismatch' }
        foreach ($t in $r.triggers) { foreach ($d in $t.data_items) {
            if ($d.byte_count*2 -ne $d.raw_hex.Length) { throw 'Hex length mismatch' }
        }}
    }
    return $r
}
$text=(& $Tests json) -join "`n"
if ($LASTEXITCODE -ne 0) { throw 'Synthetic JSON fixture failed' }
$fixture=Validate-Response $text
if ($fixture.service.name -ne '服務' -or $fixture.service.display_name -ne "サービス😀`n`"\") { throw 'Unicode/control roundtrip failed' }
if ($fixture.triggers[0].data_items[3].unsigned_value -ne '18446744073709551615') { throw '64-bit precision lost' }
$errorText=(& $App --json service-trigger-explainer-nonexistent-6ba67df8) -join "`n"
if ($LASTEXITCODE -ne 3) { throw 'Service-not-found exit code failed' }
$null=Validate-Response $errorText
# Locate a readable existing service; no specific service is required.
$inspected=0
foreach ($service in Get-Service) {
    $text=(& $App --json $service.Name) -join "`n"
    if ($LASTEXITCODE -eq 0) { $null=Validate-Response $text; $inspected++ }
    elseif ($LASTEXITCODE -ne 4 -and $LASTEXITCODE -ne 3) { throw "Unexpected inspection failure: $text" }
}
if ($inspected -eq 0) { throw 'No readable service found for JSON validation' }
Write-Output "Independent JSON parsing/schema validation passed: synthetic fixture, errors, $inspected real services."
