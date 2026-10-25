# 시나리오 모드: config\Scenario.json 순서대로 비행하며 EEG 기록
# EEG 서버 주소 (IP:포트). 원격 서버인 경우 서버 PC의 IP로 설정
$Server = "127.0.0.1:9800"

# VC++ 재배포 패키지가 없거나 오래됐으면 설치 (관리자 권한 확인 창이 뜸)
$Redist = Join-Path $PSScriptRoot "Engine\Extras\Redist\en-us\vc_redist.x64.exe"
$Installed = (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\X64" -ErrorAction SilentlyContinue).Version
$Required = (Get-Item $Redist).VersionInfo.ProductVersion
if (-not $Installed -or [version]$Installed.TrimStart("v") -lt [version]$Required) {
    Write-Host "VC++ 재배포 패키지를 설치합니다..."
    Start-Process $Redist -ArgumentList "/install", "/quiet", "/norestart" -Verb RunAs -Wait
}

Start-Process (Join-Path $PSScriptRoot "DroneSim.exe") -ArgumentList "-nosplash", "-recording", "-EegServer=$Server"
