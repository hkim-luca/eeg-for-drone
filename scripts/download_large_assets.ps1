<#
    대용량 에셋 다운로드 스크립트

    git 저장소에는 100MB 초과 파일을 넣을 수 없어, 위성 텍스처 등 대용량
    바이너리 에셋은 GitHub Release 에 올려두고 이 스크립트로 내려받는다.
    대상 목록은 scripts/large_assets.json 매니페스트에서 읽는다.
    임시 파일(.part)로 받은 뒤 크기를 검증하고 최종 경로로 옮긴다.
#>
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
$Manifest = Get-Content (Join-Path $PSScriptRoot 'large_assets.json') -Raw | ConvertFrom-Json
$BaseUrl  = "https://github.com/$($Manifest.repo)/releases/download/$($Manifest.tag)"

foreach ($a in $Manifest.assets) {
    $dest = Join-Path $RepoRoot $a.dest

    if ((Test-Path $dest) -and ((Get-Item $dest).Length -eq $a.size)) {
        Write-Host "[skip] $($a.name) — 이미 존재 (올바른 크기)"
        continue
    }

    $destDir = Split-Path -Parent $dest
    if (-not (Test-Path $destDir)) {
        New-Item -ItemType Directory -Force -Path $destDir | Out-Null
    }

    $part = "$dest.part"
    $url  = "$BaseUrl/$($a.name)"
    Write-Host "[down] $($a.name)  <-  $url"
    curl.exe -L --fail -o "$part" "$url"

    $got = (Get-Item $part).Length
    if ($got -ne $a.size) {
        Remove-Item $part -Force
        throw "$($a.name) 크기 불일치: 기대 $($a.size) / 실제 $got"
    }

    $hash = (Get-FileHash -Algorithm SHA256 -Path $part).Hash.ToLower()
    if ($hash -ne $a.sha256.ToLower()) {
        Remove-Item $part -Force
        throw "$($a.name) SHA256 불일치: 기대 $($a.sha256) / 실제 $hash"
    }

    Move-Item -Force -Path $part -Destination $dest
    Write-Host "[done] $dest ($got bytes, sha256 검증됨)"
}

Write-Host "모든 대용량 에셋 준비 완료."
