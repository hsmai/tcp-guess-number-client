# Number Guessing Game (TCP) — Auto Binary Search Client in C

본 프로젝트는 **C 언어 TCP 소켓 프로그래밍 실습**으로, 주어진 Python 서버(`numgame_server.py`)와 통신하여 서버의 정답 숫자를 **자동으로 추측**하는 클라이언트(`numgame_client.c`)를 구현합니다.

클라이언트는 사용자의 입력 없이 서버 피드백 **Higher / Lower / Correct**를 기반으로 다음 추측값을 계산하며, **이진 탐색(Binary Search)** 방식으로 **약 log₂(N)** 번의 시도 내에 정답을 찾는 것을 목표로 합니다.

---

## 프로젝트 개요

- 서버(`numgame_server.py`)는 클라이언트가 접속하면
  1) 사용자 이름(username)을 수신하고  
  2) 게임 범위 메시지 `RANGE <min> <max>`를 전송한 뒤  
  3) 클라이언트의 숫자 추측(guess)에 대해 `Higher!`, `Lower!`, `Correct! ...` 피드백을 반환합니다.

- 클라이언트(`numgame_client.c`)는
  - 서버로부터 받은 범위(min~max)에서 **이진 탐색**으로 추측값을 생성하고,
  - 각 피드백에 따라 탐색 구간을 업데이트하며,
  - `Correct!` 메시지를 받을 때까지 자동으로 반복합니다.

(참고) 사용자 입력용 클라이언트(`numgame_client_input.py`)는 게임 동작 이해를 위한 참고 코드입니다.

---

## Protocol (Server ↔ Client)

이 프로젝트는 “텍스트 메시지 + 개행(\n)” 기반의 단순 라인 프로토콜을 사용합니다.

1) 연결 직후: username 전송
    Client → Server: <username>\n

2) 서버가 범위 전송
    Server → Client: RANGE <min> <max>\n

3) 게임 루프: 추측값 전송 → 피드백 수신
    Client → Server: <guess>\n
    Server → Client: Higher!\n   (guess < target)
                 또는 Lower!\n    (guess > target)
                 또는 Correct! ...\n (guess == target)

4) 정답 시 종료
- 서버는 `Correct! ...`를 보내고, 서버 콘솔에 아래 형식 로그가 출력되어야 정답 처리됩니다.
    RESULT:CORRECT USER=<username> ATTEMPTS=<attempts>

- 클라이언트는 `Correct!` 수신 후 정상 종료해야 합니다.

---

## 코드 동작 분석

### 1) Server: `numgame_server.py` (Python)
- TCP 서버 소켓 생성 및 대기
  - `SO_REUSEADDR` 설정으로 재실행 시 포트 점유 문제 완화
  - `bind(host, port)` → `listen(5)` → `accept()`

- 클라이언트 처리 흐름(연결 1개당 1게임)
  1) `username = conn.recv(...).decode().strip()` 로 username 수신
  2) 서버가 랜덤 정답 `target = random.randint(min_num, max_num)` 생성
  3) `RANGE min max\n` 전송
  4) 클라이언트 guess 수신 → 비교 후 피드백 전송
     - guess < target → `Higher!\n`
     - guess > target → `Lower!\n`
     - guess == target → `Correct! The answer is ... attempts ...\n`
       그리고 서버 콘솔에 결과 로그 출력:
         RESULT:CORRECT USER=<username> ATTEMPTS=<attempts>

- 서버는 `while True:`로 계속 살아있으며, 여러 클라이언트를 순차적으로 처리합니다.

### 2) Client: `numgame_client.c` (C)
- 실행 인자 형식(반드시 준수)
    ./numgame_client <server_ip> <port> <username>

- 연결 및 초기 핸드셰이크
  1) `socket(PF_INET, SOCK_STREAM, 0)` 로 TCP 소켓 생성
  2) `serv_addr`에 IP/PORT 설정 (`inet_aton`, `htons`)
  3) `connect()`로 서버 접속
  4) username을 서버로 전송 (개행 포함)
     - 실제 구현: `snprintf(send_buf, "%s\n", username); send(...)`

- 서버 범위 메시지 수신 및 파싱
  - `recv()`로 `RANGE min max` 수신
  - `sscanf(recv_buf, "RANGE %d %d", &minimum_num, &maximum_num)`로 범위 파싱

- 자동 추측 로직: Binary Search
  - 초기값:
    - `low = minimum_num`, `high = maximum_num`, `attempt = 0`
  - 반복:
    1) `mid = low + (high - low) / 2` 계산
    2) `mid`를 서버로 전송 (`"%d\n"` 형태)
    3) 서버 피드백 `recv()`로 수신
    4) 피드백에 따라 구간 업데이트
       - "Higher" → `low = mid + 1`
       - "Lower"  → `high = mid - 1`
       - "Correct"→ 종료
  - 정답(`Correct`) 수신 시 루프 종료 후 `close(sock)`로 정상 종료

- 시도 횟수는 이진 탐색 특성상 평균/최대가 약 log₂(N) 수준으로 감소합니다.

---

## 실행 방법

### 1) 서버 실행
예: 1~100 범위 게임을 포트 8880에서 실행

    python3 numgame_server.py --host 127.0.0.1 --port 8880 --max_num 100

(옵션) 최소값을 바꾸고 싶다면 `--min_num`을 사용할 수 있습니다.

    python3 numgame_server.py --host 127.0.0.1 --port 8880 --min_num 1 --max_num 100

### 2) 클라이언트 컴파일

    gcc numgame_client.c -o numgame_client

### 3) 클라이언트 실행
실행 옵션 형식: `./numgame_client <server_ip> <port> <username>`

    ./numgame_client 127.0.0.1 8880 John

---

## 실행 예시

클라이언트 출력 예시(형식은 달라도 됨 — 단, Correct 수신 후 정상 종료 필수)

    Server Connected 127.0.0.1:8880
    Connected user : [John]
    Server : RANGE 1 ~ 100
    Client guess : 50
    Server : Lower!
    Client guess : 25
    Server : Higher!
    ...
    Client guess : 4
    Server : Correct! The answer is 4. You guessed it in 6 attempts.

서버 출력 예시(서버 측에 반드시 RESULT:CORRECT 로그가 찍혀야 정답 처리)

    Number Guessing Game Server started on 127.0.0.1:8880
    Client connected: ('127.0.0.1', 58622)
    [John] joined the game.
    RESULT:CORRECT USER=John ATTEMPTS=6

    [John] disconnected after 6 attempts.

---

## Repository Structure

- numgame_server.py
  - Number Guessing Game TCP 서버 (범위 전송 + Higher/Lower/Correct 피드백)
- numgame_client.c
  - 자동 이진탐색 TCP 클라이언트 (username 전송 → RANGE 파싱 → binary search guess loop)

(참고 코드로 `numgame_client_input.py`를 함께 두는 경우, 별도 폴더나 `examples/`에 두는 것을 권장합니다.)

---

## Implementation Notes

- 메시지는 개행(`\n`)을 포함해 전송합니다.
- 서버/클라이언트 모두 `BUF_SIZE=1024` 버퍼를 사용하며, 본 과제의 짧은 메시지에서는 안전하게 동작합니다.
- 클라이언트는 서버 응답을 `strncmp()`로 판별합니다.
  - "Higher", "Lower", "Correct" 키워드가 포함된 응답을 처리합니다.

---

## Troubleshooting

- 포트가 이미 사용 중인 경우
  - 다른 포트로 실행하거나 기존 프로세스를 종료하세요.
  - 예: `--port 8881`

- Correct를 받지 못하고 종료되는 경우
  - 추측값 전송 형식이 `<number>\n`인지 확인하세요.
  - RANGE 파싱이 정상인지(예: `RANGE 1 100`) 출력 로그로 확인하세요.

- 서버에서 RESULT:CORRECT가 출력되지 않는 경우
  - 클라이언트가 `Correct!` 수신 전에 연결이 끊기지 않았는지 확인하세요.




