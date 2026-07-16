# Website Status Monitor

A lightweight, native Windows C++ application to monitor the status of multiple websites. It parses target URLs, performs parallel HTTP/HTTPS checks using the native Windows WinHTTP APIs, logs output with terminal color codes, and sends Telegram alerts on status transitions.

## Features

- Native Windows WinHTTP: Built directly on Windows WinHTTP APIs, ensuring zero external dependencies (no need to link libcurl or openssl).
- Parallel Monitoring: Monitors multiple websites simultaneously using asynchronous threads (std::async) to prevent slow servers from blocking other checks.
- Latency Tracking: Measures and displays the response time in milliseconds for each website check.
- Retry Mechanism: Triggers Telegram alerts only on the third consecutive failure, minimizing false alarms from temporary network glitches.
- Auto-Reload Configuration: Automatically monitors config.txt for disk modifications and reloads the monitored list and intervals on the fly without resetting state history for unchanged websites.
- Thread-Safe Logging: Uses standard mutexes to prevent log collision in multi-threaded executions and appends clean, timestamped logs to monitor.log.
- Colorized Terminal Logs: Green output indicates UP (HTTP 200), red indicates DOWN (with detailed WinHTTP error codes or bad status codes).
- Configurable: Adjust polling interval and website lists directly in config.txt.

## How It Works

1. Configuration Loader: The program loads config.txt, parses key-value pairs (interval, Telegram token, Telegram chat ID), and reads the target URLs.
2. File Watcher: Tracks the last write time of config.txt using std::filesystem. When a change is detected, it reloads config.txt, mapping previous state records to existing targets.
3. Async Check Loop:
   - For each URL, standard std::async is used to spin up a request thread.
   - Measures time duration using std::chrono::high_resolution_clock.
   - WinHttpOpen, WinHttpConnect, WinHttpOpenRequest, WinHttpSendRequest, and WinHttpReceiveResponse are used to check the target.
   - WinHttpQueryHeaders retrieves the HTTP response code.
4. Alerts: If a Telegram token and chat ID are configured, the tool automatically url-encodes messages and sends GET requests to the Telegram Bot API when a monitored website goes offline (after 3 consecutive failures) or comes back online.

## Prerequisites

- Windows Operating System
- Visual Studio Build Tools (MSVC compiler cl.exe and VsDevCmd.bat environment)

## IDE Setup

The repository includes configuration files to automatically set up C++17 diagnostics in your code editor:
- .vscode/c_cpp_properties.json: Configures standard IntelliSense for Visual Studio Code and Cursor.
- compile_flags.txt: Configures clangd-based language servers.

These files ensure that standard filesystem classes (std::filesystem) are resolved correctly by your editor's static analysis.

## Setup and Configuration

Configure the application by editing config.txt in the root folder:

```text
# Interval in seconds between checks
interval=10

# Optional Telegram credentials for notifications (leave empty to disable)
telegram_token=YOUR_TELEGRAM_BOT_TOKEN
telegram_chat_id=YOUR_CHAT_ID_OR_CHANNEL_ID

# Websites to monitor (one per line, must start with http:// or https://)
https://www.google.com
https://www.github.com
https://httpbin.org/status/200
https://httpbin.org/status/404
https://invalid-domain-name-testing-123.org
```

## Compilation

Run the included batch script to compile the application:

```cmd
build.bat
```

This script automatically locates the Visual Studio Build Tools, initializes the compilation environment, and compiles main.cpp into uptimesite.exe.

## Execution

Launch the compiled monitor:

```cmd
uptimesite.exe
```

### Example Console Output

```text
===========================================
        Website Status Monitor v1.0        
                 by fa33az                 
   https://github.com/fa33az/uptimesite    
===========================================
[+] Monitoring 5 websites every 10 seconds.
[-] Telegram alerts disabled (credentials missing in config.txt).
-------------------------------------------
[2026-07-16 15:01:19] [UP]   https://www.google.com - HTTP 200 (216 ms)
[2026-07-16 15:01:19] [UP]   https://www.github.com - HTTP 200 (286 ms)
[2026-07-16 15:01:20] [DOWN] https://httpbin.org/status/200 - Error: HTTP Status 503 (Failure 1/3, 1089 ms)
[2026-07-16 15:01:20] [DOWN] https://httpbin.org/status/404 - Error: HTTP Status 503 (Failure 1/3, 1089 ms)
[2026-07-16 15:01:20] [DOWN] https://invalid-domain-name-testing-123.org - Error: WinHttpSendRequest failed (12007) (Failure 1/3, 104 ms)
[*] Config file changed. Reloading configuration...
[+] Reloaded 6 websites. Interval: 10s.
-------------------------------------------
[2026-07-16 15:01:30] [UP]   https://www.google.com - HTTP 200 (147 ms)
```
