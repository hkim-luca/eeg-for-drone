# config

실행 시 한 번 읽는다. 수정 후 다시 실행.

- `Scenario.json` — 시나리오 모드 비행 순서. `action`: `forward`/`backward`/`left`/`right`, `duration`: 초.
- `openbci_to_sim.json` — 보드 채널 번호 → 전극 이름.
- `sim_to_model.json` — 전극 이름 → 모델 입력 인덱스.
- `ai_model_config.json` — AI 모델 설정(클래스, 체크포인트, 입력 크기, 출력 → 드론 동작).

## 채널 흐름

```
OpenBCI ─(openbci_to_sim)→ 전극 이름 ─(sim_to_model)→ 모델 입력 → 서버 → AI 모델
```

이름이 같은 채널만 모델 입력에 들어가고, 나머지 입력은 0.

## AI 모델 교체

`eeg-server/code/`와 체크포인트를 교체하고 `ai_model_config.json`을 맞춘다.
