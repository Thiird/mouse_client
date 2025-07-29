#include "../include/com_port.hpp"
#include <windows.h>
#include <iostream>
#include <regex>

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
void read_data_mouse() {
    std::cout << "[Mouse handler]: reading data...\n";
}
void read_data_receiver() {
    std::cout << "[Receiver handler]: reading data...\n";
}

DataReadFunc get_current_read_func() {
    return read_func;
}

