# eeg-server — DroneSim dummy EEG 추론 서버

DroneSim running 모드가 접속하는 dummy EEG 서버입니다. DroneSim이 보내는
32전극 EEG frame(현재는 시뮬레이션 신호)을 데모 규칙으로 분류해
`EScenarioAction`으로 돌려주고, 평가지표를 WebUI dashboard로 제공합니다.
실제 EEG AI 모델이 준비되면 `eeg_server/inference.py`의 `classify_frame`만
교체하면 됩니다.

## 실행

```powershell
pip install -r requirements.txt
python -m eeg_server
```

- DroneSim 링크: `tcp://127.0.0.1:9800` (length-prefixed protobuf, `proto/eeg_link.proto`)
- Dashboard: <http://127.0.0.1:8800/> — 분류 정확도, 반응속도(추론→제어),
  전체 경로 지연(device→제어), 통신 신뢰도, action 확률 시계열, 32채널 파형 표시
  (두 차트의 x축은 KST)
- 시계열 로그: 추론 결과마다 dashboard 값들이 `logs/eeg_metrics_<시각>.csv`에
  저장됨 — 첫 컬럼은 KST(`YYYY-MM-DD HH:MM:SS.mmm+09:00`), 이후
  true/inferred action, confidence, action별 확률 %, 정확도, 지연(ms), 신뢰도 %

## 테스트

```powershell
python tests/test_inference.py
python tests/test_metrics.py

# E2E: 서버를 먼저 띄운 뒤, DroneSim을 흉내내는 가짜 클라이언트로 확인
python tools/fake_dronesim.py --seconds 10
```

## 프로토콜

`proto/eeg_link.proto`가 단일 스키마 소스입니다. TCP 위에서 각 메시지 앞에
4바이트 little-endian 길이를 붙입니다.

- DroneSim → 서버: `ClientMessage{ eeg: EegFrame }` (100 ms, 32ch x 25샘플),
  `ClientMessage{ ack: ControlAck }` (제어 적용 시각 확인)
- 서버 → DroneSim: `ServerMessage{ action: ActionResult }` — 추론된 action,
  rolling 분류 정확도(`accuracy_percent`), 그리고 **action별 확률 분포**
  (`action_probs`, FORWARD/BACKWARD/LEFT/RIGHT/STOP 고정 순서, 합≈1) 포함.
  confidence는 승자 action의 확률과 같음. 확률 시계열은 dashboard의
  "Action 확률" 차트에서 최근 30초 구간으로 확인

Python 코드는 protoc 생성물(`eeg_server/eeg_link_pb2.py`)을 사용하고,
UE 쪽은 `DroneSim/Source/DroneSim/Eeg/EegProto.cpp`가 같은 wire format을
직접 구현합니다(엔진에 protoc/libprotobuf 의존성을 넣지 않기 위함).
스키마 변경 시 세 곳을 함께 수정하고 아래로 재생성하세요:

```powershell
pip install grpcio-tools
python -m grpc_tools.protoc -I proto --python_out=eeg_server proto/eeg_link.proto
```

## 데모 분류 규칙

DroneSim의 신호 시뮬레이터는 action마다 정해진 채널 그룹의 진폭을 키웁니다
(`FORWARD`=ch0–5, `LEFT`=ch8–13, `RIGHT`=ch18–23, `BACKWARD`=ch26–31).
서버는 그룹별 RMS를 평균으로 정규화한 뒤 softmax로 action별 확률을 만듭니다.
`STOP`은 고정 baseline(`STOP_BIAS`)과 경쟁하므로 두드러진 그룹이 없는 휴지
신호에서는 `STOP` 확률이 우세해집니다. 추론 action은 확률의 argmax입니다.
규칙 상수는 `eeg_server/config.py`와 UE의 `EegTypes.h`에서 반드시 함께
맞춰야 합니다.
