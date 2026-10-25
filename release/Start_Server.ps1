# EEG 서버 실행. 처음 한 번은 가상환경(.venv)을 만들고 패키지를 설치한다.

Set-Location (Join-Path $PSScriptRoot "eeg-server")
$Python = ".venv\Scripts\python.exe"

if (-not (Test-Path ".venv\.ready")) {
    Write-Host "처음 실행이라 가상환경을 만들고 패키지를 설치합니다. 몇 분 걸립니다..."
    if (Test-Path ".venv") { Remove-Item ".venv" -Recurse -Force }
    if (Get-Command py -ErrorAction SilentlyContinue) { py -3 -m venv .venv } else { python -m venv .venv }
    if (-not (Test-Path $Python)) {
        Read-Host "Python을 찾을 수 없습니다. Python 3.10 이상을 설치한 뒤 다시 실행하세요. (Enter로 종료)"
        exit 1
    }
    & $Python -m pip install --disable-pip-version-check -r requirements-server.txt
    if ($LASTEXITCODE -ne 0) {
        Read-Host "패키지 설치에 실패했습니다. 인터넷 연결을 확인한 뒤 다시 실행하세요. (Enter로 종료)"
        exit 1
    }
    New-Item ".venv\.ready" -ItemType File | Out-Null
}

& $Python -m eeg_server
Read-Host "서버가 종료됐습니다. (Enter로 닫기)"
