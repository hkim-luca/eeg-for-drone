# config

DroneSim과 EEG 서버가 실행할 때 한 번 읽는 설정 파일. 수정 후 다시 실행하면 적용된다.

- `Scenario.json` — 시나리오 모드의 비행 순서. `action`은 `forward`, `backward`, `left`, `right`, `duration`은 초.
- `openbci_to_sim.json` — 보드 채널 번호 → 전극 이름. 보드의 모든 채널(Cyton 8개, Cyton+Daisy 16개)을 **실제 배치대로 직접 적는다.** 항목 수로 보드 종류를 정한다.
- `sim_to_model.json` — 전극명 → 모델 입력 인덱스(0~21). 모델이 학습된 순서대로 적는다. DroneSim이 이 이름 기준으로 신호를 모델 입력에 넣는다.
- `ai_model_config.json` — EEG 서버가 서빙할 모델(클래스, 체크포인트, 입력 크기, 출력 → 드론 동작). `checkpoint`는 `eeg-server` 기준 경로.

## 채널 흐름

```
OpenBCI(8/16ch) ─(openbci_to_sim)→ 전극명 ─(sim_to_model)→ 22ch 벡터 → 서버 → AI 모델
```

DroneSim이 샘플마다 길이 22 벡터를 만든다. 매핑되는 전극은 값을 넣고, 안 쓰는 채널은 0으로 채운다.
예: 8채널 예시(FC3,FC4,C3,Cz,C4,CP3,CP4,Pz)면 인덱스 1,5,7,9,11,13,17,19만 채워지고 나머지는 0.

채널 수가 22보다 적어 나머지를 0으로 두면 정확도는 떨어진다. 제대로 하려면 실제 채널 세트로
모델을 다시 학습하는 게 맞다.

## 모델 입력

입력 `[1, 22, 1000]` = 22채널 × 4초(250Hz), 출력 4클래스(`left_hand/right_hand/feet/tongue`).
드론 동작 매핑은 `ai_model_config.json`의 `mi_to_action`.
