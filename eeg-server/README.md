# eeg-server — DroneSim dummy EEG 추론 서버

DroneSim running 모드가 보내는 32전극 EEG frame을 데모 규칙으로 분류해
`EScenarioAction` 확률로 돌려주고, 평가지표를 dashboard와 CSV로 제공합니다.
실제 AI 모델이 준비되면 `eeg_server/inference.py`의 `classify_frame`만 교체합니다.

## 실행

```powershell
pip install -r requirements.txt
python -m eeg_server
```

- DroneSim 링크: `tcp://127.0.0.1:9800`
- Dashboard: <http://127.0.0.1:8800/>
- 시계열 로그: `logs/eeg_metrics_<시각>.csv` (첫 컬럼 KST)

## 테스트

```powershell
python tests/test_inference.py
python tests/test_metrics.py
python tools/fake_dronesim.py --seconds 10   # 서버 실행 후, DroneSim 대역 E2E
```

## 프로토콜

스키마 원본은 `proto/eeg_link.proto` 하나입니다 (TCP + 4바이트 length prefix).
Python은 생성물 `eeg_server/eeg_link_pb2.py`를 쓰고, UE는
`DroneSim/Source/DroneSim/Eeg/EegProto.cpp`가 같은 wire format을 직접 구현하므로
스키마 변경 시 셋을 함께 수정합니다. 재생성:

```powershell
python -m grpc_tools.protoc -I proto --python_out=eeg_server proto/eeg_link.proto
```

데모 분류 규칙(채널 그룹, softmax 상수)은 `eeg_server/config.py`와 UE
`EegTypes.h`에 정의되며 양쪽을 반드시 함께 맞춥니다.
