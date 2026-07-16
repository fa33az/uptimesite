#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <future>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

// Set console text color
void SetColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)color);
}

// Convert string to wstring
std::wstring ToWString(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Convert wstring to string
std::string ToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// URL structure
struct URL {
    std::string full_url;
    bool is_https = false;
    std::wstring host;
    std::wstring path;
    int port = 80;
};

// URL Parser
bool ParseURL(const std::string& raw_url, URL& parsed) {
    parsed.full_url = raw_url;
    std::string url = raw_url;
    
    // Trim leading/trailing whitespace
    url.erase(0, url.find_first_not_of(" \t\r\n"));
    url.erase(url.find_last_not_of(" \t\r\n") + 1);

    size_t proto_end = url.find("://");
    if (proto_end == std::string::npos) {
        return false;
    }
    
    std::string protocol = url.substr(0, proto_end);
    std::transform(protocol.begin(), protocol.end(), protocol.begin(), ::tolower);
    
    if (protocol == "https") {
        parsed.is_https = true;
        parsed.port = 443;
    } else if (protocol == "http") {
        parsed.is_https = false;
        parsed.port = 80;
    } else {
        return false;
    }
    
    std::string rest = url.substr(proto_end + 3);
    if (rest.empty()) return false;
    
    size_t path_start = rest.find('/');
    std::string host_part;
    std::string path_part;
    if (path_start == std::string::npos) {
        host_part = rest;
        path_part = "/";
    } else {
        host_part = rest.substr(0, path_start);
        path_part = rest.substr(path_start);
    }
    
    // Extract port if specified
    size_t colon = host_part.find(':');
    if (colon != std::string::npos) {
        std::string port_str = host_part.substr(colon + 1);
        host_part = host_part.substr(0, colon);
        try {
            parsed.port = std::stoi(port_str);
        } catch (...) {
            return false;
        }
    }
    
    parsed.host = ToWString(host_part);
    parsed.path = ToWString(path_part);
    return true;
}

// URL encoder helper for Telegram text payloads
std::string UrlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped << std::hex;
    for (char c : value) {
        if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << std::setfill('0') << (int)(unsigned char)c;
        }
    }
    return escaped.str();
}

// Native WinHTTP client to check HTTP status
bool CheckHTTPStatus(const URL& url, int& out_status_code, std::string& out_error) {
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    bool result = false;
    
    hSession = WinHttpOpen(
        L"UptimeSiteMonitor/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    
    if (!hSession) {
        out_error = "WinHttpOpen failed (" + std::to_string(GetLastError()) + ")";
        return false;
    }

    // Set connection, send and receive timeouts to 5 seconds
    WinHttpSetTimeouts(hSession, 5000, 5000, 5000, 5000);
    
    hConnect = WinHttpConnect(
        hSession,
        url.host.c_str(),
        (INTERNET_PORT)url.port,
        0
    );
    
    if (!hConnect) {
        out_error = "WinHttpConnect failed (" + std::to_string(GetLastError()) + ")";
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    DWORD flags = url.is_https ? WINHTTP_FLAG_SECURE : 0;
    hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        url.path.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags
    );
    
    if (!hRequest) {
        out_error = "WinHttpOpenRequest failed (" + std::to_string(GetLastError()) + ")";
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    // Send Request
    BOOL bResults = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0
    );
    
    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, NULL);
    } else {
        out_error = "WinHttpSendRequest failed (" + std::to_string(GetLastError()) + ")";
    }
    
    if (bResults) {
        DWORD dwStatusCode = 0;
        DWORD dwSize = sizeof(dwStatusCode);
        
        BOOL bQuery = WinHttpQueryHeaders(
            hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &dwStatusCode,
            &dwSize,
            WINHTTP_NO_HEADER_INDEX
        );
        
        if (bQuery) {
            out_status_code = (int)dwStatusCode;
            result = true;
        } else {
            out_error = "WinHttpQueryHeaders failed (" + std::to_string(GetLastError()) + ")";
        }
    } else if (out_error.empty()) {
        out_error = "WinHttpReceiveResponse failed (" + std::to_string(GetLastError()) + ")";
    }
    
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    
    return result;
}

// Telegram messenger using our native HTTP client
bool SendTelegramMessage(const std::string& token, const std::string& chat_id, const std::string& message) {
    if (token.empty() || chat_id.empty()) return false;
    
    std::string path_str = "/bot" + token + "/sendMessage?chat_id=" + chat_id + "&text=" + UrlEncode(message);
    
    URL tg_url;
    tg_url.is_https = true;
    tg_url.port = 443;
    tg_url.host = L"api.telegram.org";
    tg_url.path = ToWString(path_str);
    
    int status_code = 0;
    std::string err;
    bool success = CheckHTTPStatus(tg_url, status_code, err);
    return success && status_code == 200;
}

struct Config {
    int interval = 10;
    std::string telegram_token;
    std::string telegram_chat_id;
    std::vector<std::string> websites;
};

// Config parser
Config LoadConfig(const std::string& filename) {
    Config config;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[-] Warning: Could not open config file " << filename << ". Using defaults." << std::endl;
        return config;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        size_t equal_pos = line.find('=');
        if (equal_pos != std::string::npos) {
            std::string key = line.substr(0, equal_pos);
            std::string val = line.substr(equal_pos + 1);
            
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            val.erase(0, val.find_first_not_of(" \t"));
            val.erase(val.find_last_not_of(" \t") + 1);
            
            if (key == "interval") {
                try {
                    config.interval = std::stoi(val);
                    if (config.interval <= 0) config.interval = 10;
                } catch (...) {
                    config.interval = 10;
                }
            } else if (key == "telegram_token") {
                config.telegram_token = val;
            } else if (key == "telegram_chat_id") {
                config.telegram_chat_id = val;
            }
        } else {
            config.websites.push_back(line);
        }
    }
    return config;
}

struct SiteState {
    std::string raw_url;
    URL url;
    bool is_parsed = false;
    bool last_up = true;
    bool has_run = false;
};

struct CheckResult {
    size_t index;
    bool is_up;
    int status_code;
    std::string error;
};

int main() {
    // Print banner
    SetColor(11); // Cyan
    std::cout << "===========================================" << std::endl;
    std::cout << "        Website Status Monitor v1.0        " << std::endl;
    std::cout << "                 by fa33az                 " << std::endl;
    std::cout << "   https://github.com/fa33az/uptimesite    " << std::endl;
    std::cout << "===========================================" << std::endl;
    SetColor(7); // Reset
    
    Config config = LoadConfig("config.txt");
    
    if (config.websites.empty()) {
        SetColor(12); // Red
        std::cerr << "[-] Error: No websites to monitor in config.txt" << std::endl;
        SetColor(7);
        return 1;
    }
    
    std::vector<SiteState> sites;
    for (const auto& raw : config.websites) {
        SiteState state;
        state.raw_url = raw;
        state.is_parsed = ParseURL(raw, state.url);
        if (state.is_parsed) {
            sites.push_back(state);
        } else {
            SetColor(14); // Yellow
            std::cerr << "[-] Skipping invalid URL: " << raw << std::endl;
            SetColor(7);
        }
    }
    
    if (sites.empty()) {
        SetColor(12);
        std::cerr << "[-] Error: No valid websites found to monitor." << std::endl;
        SetColor(7);
        return 1;
    }
    
    std::cout << "[+] Monitoring " << sites.size() << " websites every " << config.interval << " seconds." << std::endl;
    if (!config.telegram_token.empty() && !config.telegram_chat_id.empty()) {
        std::cout << "[+] Telegram alerts enabled." << std::endl;
    } else {
        std::cout << "[-] Telegram alerts disabled (credentials missing in config.txt)." << std::endl;
    }
    std::cout << "-------------------------------------------" << std::endl;

    while (true) {
        std::vector<std::future<CheckResult>> futures;
        
        for (size_t i = 0; i < sites.size(); ++i) {
            futures.push_back(std::async(std::launch::async, [i, &sites]() -> CheckResult {
                CheckResult res;
                res.index = i;
                res.is_up = false;
                res.status_code = 0;
                
                int sc = 0;
                std::string err;
                bool check = CheckHTTPStatus(sites[i].url, sc, err);
                if (check && sc == 200) {
                    res.is_up = true;
                    res.status_code = sc;
                } else {
                    res.is_up = false;
                    res.status_code = sc;
                    res.error = err.empty() ? ("HTTP Status " + std::to_string(sc)) : err;
                }
                return res;
            }));
        }
        
        for (auto& f : futures) {
            CheckResult res = f.get();
            SiteState& site = sites[res.index];
            
            auto now = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);
            struct tm time_info;
            localtime_s(&time_info, &now_time);
            
            std::cout << "[" << std::put_time(&time_info, "%Y-%m-%d %H:%M:%S") << "] ";
            
            if (res.is_up) {
                SetColor(10); // Green
                std::cout << "[UP]   ";
                SetColor(7);
                std::cout << site.raw_url << " - HTTP " << res.status_code << std::endl;
                
                if (!site.last_up && site.has_run) {
                    std::string msg = "🟢 WEBSITE RECOVERED: " + site.raw_url + " is now back online (HTTP 200).";
                    SendTelegramMessage(config.telegram_token, config.telegram_chat_id, msg);
                }
                site.last_up = true;
            } else {
                SetColor(12); // Red
                std::cout << "[DOWN] ";
                SetColor(7);
                std::cout << site.raw_url << " - Error: " << res.error << std::endl;
                
                if (site.last_up || !site.has_run) {
                    std::string msg = "🔴 WEBSITE DOWN ALERT: " + site.raw_url + " is OFFLINE! Error: " + res.error;
                    SendTelegramMessage(config.telegram_token, config.telegram_chat_id, msg);
                }
                site.last_up = false;
            }
            site.has_run = true;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(config.interval));
    }
    
    return 0;
}
