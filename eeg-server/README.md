# eeg-server — DroneSim EEG 추론 서버

DroneSim가 보내는 **22채널 EEG frame**(JSON)을 버퍼링해 **4초(1000샘플) 윈도우**가 차면
**UnifiedEEGNet**(BCI IV-2a 학습)으로 추론하고, 4클래스 motor imagery를 드론 동작
(`FORWARD/BACKWARD/LEFT/RIGHT`) 확률로 매핑해 돌려줍니다. 액션은 **오직 모델 결과**로
결정됩니다(신호에 인코딩하지 않음). 평가지표는 dashboard와 CSV로 제공합니다.

## AI 모델 입력

- 모델: `UnifiedEEGNet` (`code/model.py`), 입력 `[1, 22, 1000]` = 22채널 × 4.0초 @ 250Hz
- 출력 4클래스: `left_hand / right_hand / feet / tongue` → `config.MI_TO_ACTION`로 드론 동작에 매핑
- 체크포인트: `checkpoints/loso_target_A01..A09_best.pth` (기본 A01, `config.MODEL_CHECKPOINT`)
- 라이브 전처리: 윈도우별 채널 z-score(leak-free). 학습 평가의 RA/IM-TTA는 다수 trial이 필요해
  단일 라이브 윈도우에는 적용하지 않음.
- 채널 순서는 `mappings/sim_to_model.json` / UE `EegTypes.h`와 일치해야 함.

> PyTorch/numpy 또는 체크포인트가 없으면 서버는 그대로 뜨되 **동작을 보내지 않습니다**
> (`inference.StreamClassifier`). torch 설치 시 자동으로 실모델 경로로 전환됩니다.

## 실행

```powershell
pip install -r requirements.txt   # torch 포함(실모델용)
python -m eeg_server
```

- DroneSim 링크: `tcp://127.0.0.1:9800`
- Dashboard: <http://127.0.0.1:8800/>
- 시계열 로그: `logs/eeg_metrics_<시각>.csv` (첫 컬럼 KST)

## 테스트

```powershell
python tests/test_protocol.py        # JSON wire round-trip
python tests/test_model_runtime.py   # 윈도우 버퍼 + MI→동작 매핑
python tests/test_metrics.py
```

## 프로토콜 (JSON)

TCP + 4바이트 little-endian length prefix 뒤에 **UTF-8 JSON** 객체. 메시지는 `type`으로 구분:

- 클라이언트→서버: `eeg`(프레임), `ack`(제어 확인), `physics`(물리 설정)
- 서버→클라이언트: `action`(추론 결과 + 지표)

필드 이름/타입은 `proto/eeg_link.proto`에 스키마 참고용으로 문서화돼 있습니다(와이어는 protobuf가
아니라 JSON). 구현은 Python `eeg_server/protocol.py`, UE `DroneSim/.../Eeg/EegProto.cpp`이며
변경 시 함께 맞춥니다.

## 채널 매핑

OpenBCI(8ch) → 전극명 → 모델 인덱스(0..21, 나머지 zero-fill) 매핑은 저장소 루트 `mappings/`
(`openbci_to_sim.json`, `sim_to_model.json`)에 있고 UE 취득측이 사용합니다.
