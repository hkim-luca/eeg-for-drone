# 배포 zip 생성: UE Shipping 빌드 → release/ 조립 → zip
#   powershell -ExecutionPolicy Bypass -File tools\package_release.ps1 -Name EEG-DroneSim-v1.0.3
# release/ 의 DroneSim.exe, Engine, DroneSim, eeg-server 는 이 스크립트가 채우는 생성물이며 git 에서 제외된다.
# 이 스크립트 자체는 zip 에 들어가지 않는다 (release/ 밖에 둔다).
param(
    [Parameter(Mandatory = $true)][string]$Name,
    [string]$Engine = "D:\Epic Games\UE_5.7",
    [string]$OutRoot = "D:\eeg-for-drone-dist",
    [string]$ArchiveDir = (Join-Path $OutRoot "build"),
    [switch]$SkipBuild
)
$ErrorActionPreference = "Stop"

$Repo = Split-Path -Parent $PSScriptRoot
$Release = Join-Path $Repo "release"
$SevenZip = "C:\Program Files\7-Zip\7z.exe"
$GitHubAssetLimit = 2GB

if (-not $SkipBuild) {
    & (Join-Path $Engine "Engine\Build\BatchFiles\RunUAT.bat") BuildCookRun `
        "-project=$(Join-Path $Repo 'DroneSim\DroneSim.uproject')" -platform=Win64 -clientconfig=Shipping `
        -build -cook -stage -pak -iostore -compressed -prereqs `
        "-map=/Game/Maps/Daejeon_PCG_Work" `
        -archive "-archivedirectory=$ArchiveDir" -nop4 -utf8output
    if ($LASTEXITCODE -ne 0) { throw "BuildCookRun 실패 ($LASTEXITCODE)" }
}

$Built = Join-Path $ArchiveDir "Windows"
if (-not (Test-Path $Built)) { throw "UE 빌드 결과가 없다: $Built" }

# 이전 생성물을 지우고 UE 결과를 release/ 로 복사 (.pdb 제외)
foreach ($Item in "DroneSim", "Engine", "DroneSim.exe", "eeg-server") {
    Remove-Item (Join-Path $Release $Item) -Recurse -Force -ErrorAction SilentlyContinue
}
foreach ($Item in "DroneSim", "Engine", "DroneSim.exe") {
    Copy-Item (Join-Path $Built $Item) $Release -Recurse
}
Get-ChildItem (Join-Path $Release "DroneSim\Binaries\Win64") -Filter *.pdb | Remove-Item

# 링크가 중단된 실행 파일(수 MB)이 배포에 들어가지 않도록 확인 (정상은 약 128 MB)
$Shipping = Join-Path $Release "DroneSim\Binaries\Win64\DroneSim-Win64-Shipping.exe"
if (-not (Test-Path $Shipping) -or (Get-Item $Shipping).Length -lt 50MB) { throw "Shipping 실행 파일이 없거나 비정상이다: $Shipping" }

# 서버는 추론에 필요한 것만 (.venv, logs 는 사용자 PC 에서 생성)
$Server = Join-Path $Release "eeg-server"
New-Item -ItemType Directory -Force $Server | Out-Null
foreach ($Item in "eeg_server", "code", "checkpoints", "requirements-server.txt", "README.md") {
    Copy-Item (Join-Path $Repo "eeg-server\$Item") $Server -Recurse
}
Get-ChildItem $Server -Recurse -Directory -Filter __pycache__ | Remove-Item -Recurse -Force

# Windows x64 배포이므로 Linux 스크립트(*.sh)와 ARM64 재배포 설치 파일은 제외
New-Item -ItemType Directory -Force $OutRoot | Out-Null
$Zip = Join-Path $OutRoot "$Name.zip"
Remove-Item $Zip -ErrorAction SilentlyContinue
Push-Location $Release
try {
    & $SevenZip a -tzip -mx=9 -mmt=on $Zip . "-xr!.gitattributes" "-xr!*.sh" "-xr!vc_redist.arm64.exe"
    if ($LASTEXITCODE -ne 0) { throw "7z 실패 ($LASTEXITCODE)" }
}
finally { Pop-Location }

$Size = (Get-Item $Zip).Length
"{0}  {1:N0} bytes" -f $Zip, $Size
if ($Size -ge $GitHubAssetLimit) { Write-Warning "GitHub Releases 자산 한도(2 GiB)를 넘는다" }
