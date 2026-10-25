EEG DroneSim

준비
  - Windows 10/11 64비트
  - Python 3.10 이상 (서버용, 설치 시 "Add python.exe to PATH" 체크)
  - NVIDIA GPU 권장 (없으면 CPU로 추론)
  - OpenBCI 동글을 USB에 연결

실행 (각 파일을 우클릭 → "PowerShell에서 실행")
  1. Start_Server.ps1   EEG 서버 (처음 한 번은 PyTorch 설치로 약 3GB 다운로드)
  2. Run_DroneSim.ps1   실행 모드: AI 모델 추론으로 드론 조작
     Run_Scenario.ps1   시나리오 모드: 학습 데이터 수집 (Space 키로 기록 시작)
  VC++ 재배포 패키지가 없으면 처음 실행할 때 설치합니다 (관리자 권한 확인 창이 뜨면 허용)

서버를 다른 PC에서 실행할 때
  - DroneSim PC: Run_DroneSim.ps1, Run_Scenario.ps1 맨 위의 $Server를 서버 PC IP로 변경
                 (예: $Server = "192.168.0.10:9800")
  - 서버 PC: 처음 실행할 때 Windows 방화벽 허용 창이 뜨면 허용

설정 (config 폴더, 자세한 내용은 config\README.md)
  - Scenario.json         시나리오 모드의 비행 순서
  - openbci_to_sim.json   보드 채널별 전극 위치

기록
  - 시나리오 모드: Recordings 폴더 (<이름>.csv 위치, <이름>_eeg.csv EEG 원신호)
  - 서버 지표 로그: eeg-server\logs
  - 서버 대시보드: 서버 PC에서 http://127.0.0.1:8800/
