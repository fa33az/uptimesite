# Website Status Monitor

A lightweight, native Windows C++ application to monitor the status of multiple websites. It parses target URLs, performs parallel HTTP/HTTPS checks using the native Windows WinHTTP APIs, logs output with terminal color codes, and sends Telegram alerts on status transitions.

## Features

- Native Windows WinHTTP: Built directly on Windows WinHTTP APIs, ensuring zero external dependencies (no need to link libcurl or openssl).
- Parallel Monitoring: Monitors multiple websites simultaneously using asynchronous threads (std::async) to prevent slow servers from blocking other checks.
- State-based Telegram Alerts: Alerts are sent only when a website's state transitions (UP to DOWN, or DOWN back to UP) to avoid spamming.
- Colorized Terminal Logs: Green output indicates UP (HTTP 200), red indicates DOWN (with detailed WinHTTP error codes or bad status codes).
- Configurable: Adjust polling interval and website lists directly in config.txt.

## How It Works

1. Configuration Loader: The program loads config.txt, parses key-value pairs (interval, Telegram token, Telegram chat ID), and reads the target URLs.
2. URL Parser: Each URL is parsed into protocol, host, port, and query path components.
3. Async Check Loop:
   - For each URL, standard std::async is used to spin up a request thread.
   - WinHttpOpen, WinHttpConnect, WinHttpOpenRequest, WinHttpSendRequest, and WinHttpReceiveResponse are used to check the target.
   - WinHttpQueryHeaders retrieves the HTTP response code.
4. Alerts: If a Telegram token and chat ID are configured, the tool automatically url-encodes messages and sends GET requests to the Telegram Bot API when a monitored website goes offline or comes back online.

## Prerequisites

- Windows Operating System
- Visual Studio Build Tools (MSVC compiler cl.exe and VsDevCmd.bat environment)

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
[2026-07-16 14:53:45] [UP]   https://www.google.com - HTTP 200
[2026-07-16 14:53:45] [UP]   https://www.github.com - HTTP 200
[2026-07-16 14:53:53] [DOWN] https://httpbin.org/status/200 - Error: WinHttpReceiveResponse failed (12002)
[2026-07-16 14:53:53] [DOWN] https://httpbin.org/status/404 - Error: WinHttpReceiveResponse failed (12002)
[2026-07-16 14:53:53] [DOWN] https://invalid-domain-name-testing-123.org - Error: WinHttpSendRequest failed (12007)
```
