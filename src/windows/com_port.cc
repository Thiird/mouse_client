#include "../include/com_port.hpp"
#include <windows.h>
#include <iostream>
#include <regex>
#include <sstream>
#include <inttypes.h>


static HANDLE hSerial = INVALID_HANDLE_VALUE;
static Subject currentSubject = Subject::UNKNOWN;
static DataReadFunc read_func = nullptr;
static Platform currentPlatform = Platform::WINDOWS;  // known at compile

Platform get_current_platform() {
    return currentPlatform;
}

bool app_init() {
    currentSubject = Subject::UNKNOWN;
    read_func = nullptr;

    while (true) {
        std::cout << "Trying to open COM port...\n";
        hSerial = CreateFile(
            L"\\\\.\\COM3", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

        if (hSerial != INVALID_HANDLE_VALUE) {
            std::cout << "Connected to COM port.\n";

            // Configure
            DCB dcb = {0}; dcb.DCBlength = sizeof(dcb);
            GetCommState(hSerial, &dcb);
            dcb.BaudRate = CBR_115200;
            dcb.ByteSize = 8; dcb.StopBits = ONESTOPBIT; dcb.Parity = NOPARITY;
            SetCommState(hSerial, &dcb);

            return true;
        }
        std::cerr << "Failed. Retry in 1 sec...\n";
        Sleep(1000);
    }
}

void app_close() {
    if (hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
    }
    currentSubject = Subject::UNKNOWN;
    read_func = nullptr;
}

// Write ENTER, read response, detect subject
bool app_scan_and_detect(Subject &subject) {
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "[app_scan_and_detect] Invalid handle. Probably disconnected.\n";
        return false;
    }

    std::cout << "[app_scan_and_detect] Sending ENTER character to probe device...\n";

    const char enter = '\n';
    DWORD bw=0;
    if (!WriteFile(hSerial, &enter, 1, &bw, NULL)) {
        std::cerr << "[app_scan_and_detect] Write failed. Device may be disconnected.\n";
        return false;
    }

    char buf[2000] = {0};
    DWORD br=0;
    if (!ReadFile(hSerial, buf, sizeof(buf)-1, &br, NULL)) {
        std::cerr << "[app_scan_and_detect] Read failed. Device may be disconnected or not responding.\n";
        return false;
    }

    std::string line(buf, br);
    std::cout << "[app_scan_and_detect] Received response: \"" << line << "\"\n";

    std::smatch match;
    std::regex pattern("-+ (\\w+) COM PORT -+");

    if (std::regex_search(line, match, pattern)) {
        std::string subj = match[1];
        std::cout << "[app_scan_and_detect] Matched pattern. Subject detected: " << subj << "\n";

        if (subj == "MOUSE") {
            subject = Subject::MOUSE;
            read_func = read_data_mouse;
            std::cout << "[app_scan_and_detect] Handler set to read_data_mouse.\n";
        } else if (subj == "RECEIVER") {
            subject = Subject::RECEIVER;
            read_func = read_data_receiver;
            std::cout << "[app_scan_and_detect] Handler set to read_data_receiver.\n";
        } else {
            std::cerr << "[app_scan_and_detect] Unknown subject type: " << subj << "\n";
            subject = Subject::UNKNOWN;
        }
        return true;
    } else {
        std::cout << "[app_scan_and_detect] Pattern not found in response. Will try again.\n";
    }

    return true; // keep looping, subject still UNKNOWN
}


// Dummy handlers
void read_data_receiver() {
    std::cout << "[Receiver handler]: reading data...\n";
}

DataReadFunc get_current_read_func() {
    return read_func;
}

MouseStatus read_mouse_status() {
    MouseStatus status;

    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "[read_mouse_status] Invalid COM port handle.\n";
        return status;
    }

    // Send ENTER to trigger the status report
    const char enter = '1';
    DWORD bw=0;
    if (!WriteFile(hSerial, &enter, 1, &bw, NULL)) {
        std::cerr << "[read_mouse_status] Failed to send ENTER.\n";
        return status;
    }

    // Read response (make sure you configured timeouts)
    char buf[1024] = {0};
    DWORD br=0;
    if (!ReadFile(hSerial, buf, sizeof(buf)-1, &br, NULL)) {
        std::cerr << "[read_mouse_status] Failed to read response.\n";
        return status;
    }

    std::string response(buf, br);
    std::cout << "[read_mouse_status] Got response:\n" << response << "\n";

    std::smatch m;

    std::regex date_re(R"(Firmware build date:\s*(.*))");
    if (std::regex_search(response, m, date_re)) {
        status.firmware_build_date = m[1];
    }

    std::regex clicks_re(R"(Left clicks:\s*(\d+))");
    if (std::regex_search(response, m, clicks_re)) {
        status.left_clicks = std::stoull(m[1]);
    }

    clicks_re = std::regex(R"(Right clicks:\s*(\d+))");
    if (std::regex_search(response, m, clicks_re)) {
        status.right_clicks = std::stoull(m[1]);
    }

    clicks_re = std::regex(R"(Middle clicks:\s*(\d+))");
    if (std::regex_search(response, m, clicks_re)) {
        status.middle_clicks = std::stoull(m[1]);
    }

    clicks_re = std::regex(R"(Backward clicks:\s*(\d+))");
    if (std::regex_search(response, m, clicks_re)) {
        status.backward_clicks = std::stoull(m[1]);
    }

    clicks_re = std::regex(R"(Forward clicks:\s*(\d+))");
    if (std::regex_search(response, m, clicks_re)) {
        status.forward_clicks = std::stoull(m[1]);
    }

    clicks_re = std::regex(R"(Downward scrolls:\s*(\d+))");
    if (std::regex_search(response, m, clicks_re)) {
        status.downward_scrolls = std::stoull(m[1]);
    }

    clicks_re = std::regex(R"(Upward scrolls:\s*(\d+))");
    if (std::regex_search(response, m, clicks_re)) {
        status.upward_scrolls = std::stoull(m[1]);
    }

    std::regex battery_re(R"(Battery level:\s*(-?\d+)mV\s*=\s*(\d+)%?)");
    if (std::regex_search(response, m, battery_re)) {
        status.battery_mv = std::stoi(m[1]);
        status.battery_percent = std::stoi(m[2]);
    }

    std::regex dpi_re(R"(Current DPI \[.*\]:\s*(\d+))");
    if (std::regex_search(response, m, dpi_re)) {
        status.current_dpi = std::stoi(m[1]);
    }

    return status;
}

void read_data_mouse() {
    MouseStatus status = read_mouse_status();

    std::cout << "==================== Mouse Status ====================\n";
    std::cout << "Firmware build date: " << status.firmware_build_date << "\n";
    std::cout << "Left clicks:        " << status.left_clicks << "\n";
    std::cout << "Right clicks:       " << status.right_clicks << "\n";
    std::cout << "Middle clicks:      " << status.middle_clicks << "\n";
    std::cout << "Backward clicks:    " << status.backward_clicks << "\n";
    std::cout << "Forward clicks:     " << status.forward_clicks << "\n";
    std::cout << "Downward scrolls:   " << status.downward_scrolls << "\n";
    std::cout << "Upward scrolls:     " << status.upward_scrolls << "\n";
    std::cout << "Battery level:      " << status.battery_mv << "mV = " << status.battery_percent << "%\n";
    std::cout << "Current DPI:        " << status.current_dpi << "\n";
    std::cout << "======================================================\n";
}


