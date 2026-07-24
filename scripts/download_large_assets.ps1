<#
    대용량 에셋 다운로드 스크립트

    git 저장소에는 100MB 초과 파일을 넣을 수 없어, 위성 텍스처 등 대용량
    바이너리 에셋은 GitHub Release 에 올려두고 이 스크립트로 내려받는다.
    각 에셋을 지정된 경로에 배치하며, 이미 올바른 크기로 존재하면 건너뛴다.
#>
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
$Tag      = 'large-assets'
$BaseUrl  = "https://github.com/hkim-luca/eeg-for-drone/releases/download/$Tag"

$Assets = @(
    @{
        Name = 'daejeon_satellite_z16.uasset'
        Dest = 'DroneSim/Content/Maps/daejeon_satellite_z16.uasset'
        Size = 2058129989
    }
)

foreach ($a in $Assets) {
    $dest = Join-Path $RepoRoot $a.Dest

    if ((Test-Path $dest) -and ((Get-Item $dest).Length -eq $a.Size)) {
        Write-Host "[skip] $($a.Name) — 이미 존재 (올바른 크기)"
        continue
    }

    $destDir = Split-Path -Parent $dest
    if (-not (Test-Path $destDir)) {
        New-Item -ItemType Directory -Force -Path $destDir | Out-Null
    }

    $url = "$BaseUrl/$($a.Name)"
    Write-Host "[down] $($a.Name)  <-  $url"
    curl.exe -L --fail -o "$dest" "$url"

    $got = (Get-Item $dest).Length
    if ($got -ne $a.Size) {
        throw "$($a.Name) 크기 불일치: 기대 $($a.Size) / 실제 $got"
    }
    Write-Host "[done] $dest ($got bytes)"
}

Write-Host "모든 대용량 에셋 준비 완료."
