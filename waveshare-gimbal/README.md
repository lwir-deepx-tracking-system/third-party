
# Waveshare Gimbal C++ Test

Waveshare 2-Axis Pan-Tilt Camera Module의
Linux USB Serial 제어를 검증하는 C++ 테스트 프로그램.

## Environment
- C++17
- CMake
- Linux (WSL2 / Orange Pi)
- Serial: 115200 baud

## USB Setup (Windows + WSL2)

### 1. Windows 드라이버 설치

짐벌의 ESP32 통신용 USB 포트(보드 가운데 USB 포트)를 PC에 연결합니다.
짐벌 구동에는 별도의 12V 전원도 필요합니다.

장치 관리자에서 `CP2102N USB to UART Bridge Controller`에 오류 코드 28이
표시되면 [Silicon Labs 공식 페이지](https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers)에서
`CP210x Universal Windows Driver`를 받아 압축을 풉니다.
장치의 **드라이버 업데이트 → 내 컴퓨터에서 드라이버 찾아보기**에서
압축을 푼 폴더를 지정하고 **하위 폴더 포함**으로 설치합니다.

정상 설치되면 **포트(COM 및 LPT)** 아래
`Silicon Labs CP210x USB to UART Bridge (COM번호)`가 표시됩니다.
Windows의 COM 번호와 WSL의 `ttyUSB` 번호는 서로 다릅니다.

### 2. usbipd-win 설치 (최초 1회)

**Windows 관리자 PowerShell**에서 실행합니다. WSL 터미널에서 실행하는 명령이 아닙니다.

```powershell
winget install --interactive --exact dorssel.usbipd-win
```

설치 후 관리자 PowerShell을 닫았다가 다시 엽니다.

### 3. USB를 WSL에 연결

Ubuntu 터미널을 열어 WSL이 실행 중인 상태로 둡니다.
**Windows 관리자 PowerShell**에서 장치 목록을 확인합니다.

```powershell
usbipd list
```

`Silicon Labs CP210x` 장치의 **BUSID**를 확인합니다.
아래 `4-4`는 예시이므로 실제 BUSID로 바꿉니다.
처음 공유할 때는 다음 명령을 실행합니다.

```powershell
usbipd bind --busid 4-4
```

이어서 WSL에 연결합니다. `attach`는 일반 PowerShell에서도 실행할 수 있습니다.

```powershell
usbipd attach --wsl --busid 4-4
usbipd list
```

장치 상태가 `Attached`인지 확인합니다. `Shared`는 공유만 설정되어 있고
WSL에는 아직 연결되지 않은 상태입니다.
WSL에 연결된 동안 Windows 프로그램에서는 해당 COM 포트를 사용할 수 없습니다.

### 4. WSL 장치 경로 확인

**Ubuntu 터미널**에서 실행합니다.

```bash
ls -l /dev/serial/by-id/
ls -l /dev/ttyUSB*
```

`/dev/serial/by-id/`의 `CP2102N` 링크가 가리키는 `ttyUSB`가 사용할 장치입니다.
재연결 후 번호가 `ttyUSB0`에서 `ttyUSB1` 등으로 바뀔 수 있으므로,
아래 Hardware Test 명령과 VS Code 실행 시 입력하는 장치 경로를 맞춥니다.
`ttyUSB` 파일이 보이는 것만으로 현재 연결이 정상이라고 판단하지 말고,
Windows의 `usbipd list`에서도 `Attached`인지 확인합니다.

`Permission denied`가 발생하고 `id -nG`에 `dialout`이 없다면 실행합니다.

```bash
sudo usermod -aG dialout "$USER"
```

그룹 변경 후에는 WSL 세션을 새로 시작해야 합니다.

### 5. USB 재연결 및 연결 문제

USB를 뺐다 꽂거나 WSL을 재시작했다면, Ubuntu 터미널을 열고
Windows PowerShell에서 `usbipd list`로 BUSID를 다시 확인한 뒤
`usbipd attach --wsl --busid 4-4`를 실행합니다.
이미 `Shared` 상태라면 `bind`는 다시 할 필요가 없습니다.
연결 후에는 4번 단계에서 현재 `ttyUSB` 경로를 다시 확인합니다.

오래된 포트를 여는 중 프로그램이 멈췄다면 테스트를 중단하고 연결 상태를 확인합니다.
재연결해도 해결되지 않으면 WSL 작업을 저장하고 모든 실행 작업을 종료한 뒤,
Windows PowerShell에서 다음 명령으로 WSL을 완전히 종료합니다.
이 명령은 모든 WSL 배포판과 그 안의 프로세스를 종료합니다.

```powershell
wsl --shutdown
```

Ubuntu를 다시 열고 USB를 `attach`한 뒤 장치 경로를 확인합니다.

참고: [Microsoft WSL USB 연결 안내](https://learn.microsoft.com/en-us/windows/wsl/connect-usb)

## Build

`waveshare-gimbal` 디렉터리에서 Linux 셸로 실행합니다.
아래 명령은 CMake 설정이 성공하면 이어서 빌드를 수행합니다.

```bash
cmake -S . -B build && cmake --build build -j
```

## Test (Without Hardware)

각 명령은 별도로 실행합니다.

```bash
./build/gimbal_test --dry-run home
./build/gimbal_test --dry-run stop
./build/gimbal_test --dry-run angle 10 0
./build/gimbal_test --dry-run move 1 0
```

## Hardware Test

`/dev/ttyUSB0`를 실제 연결된 장치 경로로 바꾸고, 각 명령을 별도로 실행합니다.

```bash
./build/gimbal_test /dev/ttyUSB0 stop
./build/gimbal_test /dev/ttyUSB0 angle 10 0
./build/gimbal_test /dev/ttyUSB0 move 1 0
```

현재 프로그램은 명령 송신만 수행하며, 장치 응답이나 실제 동작은 검증하지 않습니다.

## Status

- [ ] Linux build
- [ ] JSON command generation
- [ ] Serial device connection
- [ ] Angle control
- [ ] Manual movement
- [ ] Stop
- [ ] Device feedback

## References
- https://www.waveshare.com/wiki/2-Axis_Pan-Tilt_Camera_Module
- https://github.com/waveshareteam/ugv_rpi
