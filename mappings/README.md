# mappings

OpenBCI 채널을 AI 모델 입력 형식으로 바꾸는 데 쓰는 매핑 파일.
OpenBCI와 UE는 같은 PC, eeg-server는 (운영 시) 원격이라 양쪽이 같은 값을 봐야 해서
`proto/`처럼 저장소 루트에 둔다.

## 파일

- `openbci_to_sim.json` — 각 보드 채널의 전극 이름(8/16채널). **실제 배치대로 직접 고친다.** `channel_count`를 쓰는 보드에 맞춘다.
- `sim_to_model.json` — 전극명 → 모델 입력 인덱스(0~21). 학습된 순서로 적용한다.

## 흐름

```
OpenBCI(8/16ch) ─(openbci_to_sim)→ 전극명 ─(sim_to_model)→ 22ch 벡터 → 서버 → UnifiedEEGNet
```

UE가 샘플마다 길이 22 벡터를 만든다. 매핑되는 전극은 값을 넣고, 안 쓰는 채널은 0으로 채운다.
예: 8채널 예시(FC3,FC4,C3,Cz,C4,CP3,CP4,Pz)면 인덱스 1,5,7,9,11,13,17,19만 채워지고 나머지는 0.

채널 수가 22보다 적어 나머지를 0으로 두면 정확도는 떨어진다. 데모/배선 확인용이고,
제대로 하려면 실제 채널 세트로 모델을 다시 학습하는 게 맞다.

## 모델 입력

UnifiedEEGNet, 입력 `[1, 22, 1000]` = 22채널 × 4초(250Hz), 출력 4클래스
(`left_hand/right_hand/feet/tongue`). 드론 동작 매핑은 `eeg_server/config.py`의 `MI_TO_ACTION`.

## 쓰기 전 할 일

- `openbci_to_sim.json`을 실제 전극 위치로 수정 (8채널이면 그대로, 16채널이면 `channel_count`를 16으로 올리고 8~15 추가)
- eeg-server에서 `pip install torch` (없으면 서버는 계속 STOP만 보냄)
- UE에 BrainFlow C++ SDK를 `D:\eeg-for-drone\DroneSim\ThirdParty\BrainFlow\`에 넣고 리빌드 (그 폴더 README 참고)
