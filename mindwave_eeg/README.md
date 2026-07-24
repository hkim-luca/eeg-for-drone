# mindwave_eeg — 1채널 EEG 실시간 대시보드

HR-S0C2129B(NeuroSky **TGAM / ThinkGear** 기반 1채널 EEG)의 무선 신호를
블루투스로 수신해 실시간 파형·대역파워·집중도/명상도로 표시한다.

## 장비 정보

| 항목 | 값 |
|---|---|
| 헤드셋 BT 이름 | **HR-S0C2129B** |
| 방식 | NeuroSky **TGAM** 칩 → **ThinkGear** 프로토콜 (`0xAA 0xAA` 패킷) |
| raw 샘플링 | 512 Hz (16bit signed) |
| 대역/집중도/명상도 | 1 Hz 갱신 (`0x83` / `0x04` / `0x05`) |
| USB 어댑터(hc01.com) | STM32 USB CDC, `VID:PID=0483:5740`, **COM3** |

> **주의:** USB로 잡히는 **COM3는 충전/설정 전용**이며 뇌파 데이터가 나오지 않는다
> (`CHG` LED 점등 = 충전 중). 실제 데이터는 **블루투스(SPP)** 로 전송된다.

## 블루투스 페어링 방법

1. 헤드셋 전원을 켠다 (`PWR`·`LNK` LED 점등 확인).
2. Windows **설정 → Bluetooth 및 장치 → 장치 추가 → Bluetooth**.
3. 목록에서 **`HR-S0C2129B`** 선택.
4. PIN 입력 요청 시 **`1234`** 입력. (안 되면 `0000` 시도)
5. 페어링되면 데이터용 COM 포트가 새로 생긴다 → 본 환경에서는 **COM5**.

### 데이터 포트 확인
```powershell
python -m serial.tools.list_ports -v
```
"Standard Serial over Bluetooth link"로 표시되는 포트가 데이터 포트다.
BT SPP 가상 포트이므로 **baud 값은 무시**된다(어느 값이든 동일하게 수신됨).

## 실행

```powershell
pip install -r requirements.txt
python eeg_dashboard.py        # PORT = "COM5"
```

- 상단: raw EEG 파형(512Hz, 2초 스크롤) + 신호품질
- 하단: 8개 대역파워(delta~midGamma) + 집중도/명상도
- 대역·집중도·명상도는 1초에 한 번 오므로 실행 후 1~2초 뒤 채워진다.
- 신호품질이 `GOOD`이어야 값이 유효하다 → 이마 전극 + 귀 클립 밀착.

## 파일

| 파일 | 용도 |
|---|---|
| `eeg_dashboard.py` | 실시간 통합 대시보드 (메인) |
| `thinkgear.py` | ThinkGear 패킷 파서 |
| `requirements.txt` | 의존성 (pyserial, numpy, matplotlib) |

## 트러블슈팅

- **데이터가 안 옴** → COM3(USB)가 아니라 **페어링으로 생긴 BT 포트(COM5)** 를 쓰는지 확인.
- **포트에서 0바이트** → 헤드셋 전원/`LNK` 링크 상태와 전극(이마+귀 클립) 접촉을 확인.
  ThinkGear는 링크되면 57600에서 `AA AA ...` 패킷을 자동 송신한다(baud는 SPP라 무시됨).
- **파형은 뜨는데 창이 안 움직임(드래그 불가)** → 해결됨. 백그라운드 리더 스레드를
  제거하고, 시리얼 읽기를 애니메이션 콜백(메인 스레드) 안에서 `timeout=0` 논블로킹으로만
  수행하도록 단일 스레드화함. 메인 스레드를 붙잡을 경쟁 스레드가 없다.
