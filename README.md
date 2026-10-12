# SysMon — Lightweight Modular Windows System Monitor & Remote Telemetry Payload

[![C++17](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows%2010%2F11-0078D6.svg)](https://microsoft.com/windows)
[![Build System](https://img.shields.io/badge/Build-CMake-064F8C.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

**SysMon** is a high-performance, modular C++17 native Windows system monitoring tool and telemetry framework. Built specifically for lightweight execution, it captures window context, keystroke input, clipboard contents, and multi-monitor desktop screenshots, exfiltrating telemetry data via encrypted WinHTTP connections to a Telegram C2 Bot API endpoint.

---

## 🌟 Key Features

* **⌨️ Low-Level Input Monitoring (`WH_KEYBOARD_LL`)**:
  * Captures global system keystrokes across all applications.
  * Accurately tracks `Shift`, `CapsLock`, and printable/special key combinations.
  * Tracks foreground window titles (`GetForegroundWindow` / `GetWindowTextA`) to supply application context.

* **📋 Real-Time Clipboard Tracker**:
  * Runs a dedicated background thread monitoring the Win32 clipboard viewer chain (`WM_DRAWCLIPBOARD`).
  * Captures and logs text copied to the clipboard.

* **📸 GDI+ Multi-Monitor Screenshot Engine**:
  * Periodically captures high-resolution desktop frames using native Windows GDI+.
  * Supports virtual multi-monitor setups (`SM_CXVIRTUALSCREEN`).
  * Encodes screenshots to PNG format in-memory and uploads them as native photos.

* **⚡ Optimized WinHTTP HTTPS Exfiltration**:
  * Reuses `HINTERNET` connection handles to eliminate TLS handshake overhead.
  * Employs non-blocking, thread-safe buffering to prevent input lag.

* **🛡️ Persistence & Self-Healing Watchdog**:
  * Multi-stage persistence via HKCU/HKLM Registry Run keys, Windows Scheduled Tasks, and Startup scripts.
  * Automated Windows Defender exclusions via PowerShell (`Add-MpPreference`).
  * Periodic watchdog loop that verifies process integrity and restores missing main executables from cold disk backups.

* **🎮 Telegram C2 Command Listener & Remote Self-Destruct**:
  * Efficient long-polling listener using Telegram `getUpdates` with persistent offset storage (`offset.dat`).
  * Supports remote `!nuke` command to terminate processes, un-install startup registry/tasks, erase all disk replicas, and self-delete the binary cleanly.

---

## 🏗️ Project Architecture

```
sysMon/
├── CMakeLists.txt             # CMake Build Configuration
├── include/                   # Modular C++ Header Declarations
│   ├── config.h               # Configuration, Pragmas & Shared Globals
│   ├── telegram.h             # Telegram API WinHTTP Exfiltration Interface
│   ├── persistence.h          # Registry, Task Scheduler & Watchdog Interface
│   ├── keylogger.h            # Keyboard Hook & Clipboard Tracker Declarations
│   ├── screenshot.h           # GDI+ Desktop Screen Capture Declarations
│   └── c2_listener.h          # Remote Command Polling & Nuke Declarations
└── src/                       # Module Implementations
    ├── main.cpp               # Lightweight Entry Point (WinMain & Watchdog)
    ├── crypto.cpp             # XOR Configuration Decryption
    ├── telegram.cpp           # HTTPS WinHTTP API Implementation
    ├── persistence.cpp        # Persistence, Replacements & Defender Rules
    ├── keylogger.cpp          # Keylog Hook, Window Tracker & Clipboard Logic
    ├── screenshot.cpp         # GDI+ PNG Screen Capture Engine
    └── c2_listener.cpp        # Telegram C2 Listener & Self-Destruct Routine
```

---

## 🔨 Building & Compilation

### Prerequisites
* **Windows SDK 10/11**
* **CMake 3.10+**
* **Compiler**: Microsoft Visual C++ (MSVC 2019/2022) or MinGW-w64 (GCC 10+)

### Building with CMake (MSVC / Visual Studio)

```powershell
# 1. Clone the repository
git clone git@github-curry06:Curry06/sysMon.git
cd sysMon

# 2. Create build directory
mkdir build
cd build

# 3. Generate Build Files & Compile Release Binary
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### Direct MSVC Command-Line Compilation (`cl.exe`)

```cmd
cl.exe /O2 /EHsc /Iinclude src\*.cpp /link winhttp.lib gdiplus.lib ole32.lib psapi.lib ws2_32.lib shell32.lib advapi32.lib /OUT:SysMon.exe /SUBSYSTEM:WINDOWS
```

---

## 📡 Remote C2 Telegram Commands

When the bot is active, operators can issue commands directly through the Telegram chat:

| Command | Action | Description |
| :--- | :--- | :--- |
| `!nuke` | **Remote Self-Destruct** | Immediately stops all hooks, deletes scheduled tasks, clears registry autostarts, erases all cold backup replicas, and self-deletes the executable from disk via a temporary script. |

---

## ⚠️ Disclaimer & Legal Notice

This software is developed strictly for **educational, security research, and authorized telemetry/auditing purposes**. Unauthorized deployment of monitoring software on systems without explicit consent is illegal. The author assumes no liability for misuse or damage caused by this program.

---

## 📜 License

Distributed under the [MIT License](LICENSE).
