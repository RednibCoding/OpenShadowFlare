$sfRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\ShadowFlare')).Path
$outRoot = Join-Path $PSScriptRoot '..\decoded'
$decoderSrc = Join-Path $PSScriptRoot 'decode.c'
$decoder = Join-Path $PSScriptRoot 'decode.exe'

& gcc $decoderSrc -O2 -o $decoder
if ($LASTEXITCODE -ne 0) {
    throw "gcc failed while building $decoderSrc"
}

if (-not (Test-Path -LiteralPath $outRoot)) {
    New-Item -ItemType Directory -Path $outRoot -Force | Out-Null
}

$files = Get-ChildItem -Path $sfRoot -Recurse -Filter "*.Njp"
$total = $files.Count
$i = 0

foreach ($file in $files) {
    $i++
    $relPath = $file.FullName.Substring($sfRoot.Length + 1)
    $outPath = Join-Path $outRoot ([IO.Path]::ChangeExtension($relPath, '.bmp'))
    $outDir = Split-Path $outPath -Parent

    if (-not (Test-Path $outDir)) {
        New-Item -ItemType Directory -Path $outDir -Force | Out-Null
    }

    Write-Host "[$i/$total] $relPath"
    & $decoder $file.FullName $outPath
}

Write-Host ""
Write-Host "Done. Output in: $outRoot"
