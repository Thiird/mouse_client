#include "../include/com_port.hpp"
#include <windows.h>
#include <iostream>
#include <regex>
#include <sstream>
#include <inttypes.h>
#include <QApplication>
#include "../include/gui.hpp"
#include <expected>

static HANDLE hSerial = INVALID_HANDLE_VALUE;

namespace ComPort
{
    bool ret = false;

    bool connect()
    {
        currentSubject = Subject::UNKNOWN;
        read_func = nullptr;

        std::cout << "Trying to open COM port...\n";
        hSerial = CreateFile(L"\\\\.\\COM3", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

        if (hSerial != INVALID_HANDLE_VALUE)
        {
            std::cout << "Connected to COM port.\n";

            // Configure
            DCB dcb = {0};
            dcb.DCBlength = sizeof(dcb);
            ret = GetCommState(hSerial, &dcb);
            if (!ret)
            {
                std::cout << "GetCommState failed" << std::endl;
                return false;
            }
            dcb.BaudRate = CBR_115200;
            dcb.ByteSize = 8;
            dcb.StopBits = ONESTOPBIT;
            dcb.Parity = NOPARITY;
            ret = SetCommState(hSerial, &dcb);
            if (!ret)
            {
                std::cout << "GetCommState failed" << std::endl;
                return false;
            }

            COMMTIMEOUTS timeouts = {0};
            timeouts.ReadIntervalTimeout = 50;
            timeouts.ReadTotalTimeoutConstant = 50;
            timeouts.ReadTotalTimeoutMultiplier = 10;
            timeouts.WriteTotalTimeoutConstant = 50;
            timeouts.WriteTotalTimeoutMultiplier = 10;

            if (!SetCommTimeouts(hSerial, &timeouts))
            {
                std::cerr << "SetCommTimeouts failed!" << std::endl;
                return false;
            }

            return true;
        }

        return false;
    }

    void disconnect()
    {
        if (hSerial != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;
        }
        currentSubject = Subject::UNKNOWN;
        read_func = nullptr;
    }

    bool read_mouse_data(MouseStatus &status)
    {
        // Send ENTER to trigger the status report
        const char enter = '1';
        DWORD bw = 0;
        if (!WriteFile(hSerial, &enter, 1, &bw, NULL))
        {
            appendLogMessage("Failed to send ENTER", Qt::black);
            return false;
        }

        // Read response (make sure you configured timeouts)
        char buf[1024] = {0};
        DWORD br = 0;
        if (!ReadFile(hSerial, buf, sizeof(buf) - 1, &br, NULL))
        {
            return false;
        }

        std::string response(buf, br);
        std::smatch m;

        std::regex date_re(R"(Firmware build date:\s*(.*))");
        if (std::regex_search(response, m, date_re))
        {
            status.firmware_build_date = m[1];
        }

        std::regex clicks_re(R"(Left clicks:\s*(\d+))");
        if (std::regex_search(response, m, clicks_re))
        {
            status.left_clicks = std::stoull(m[1]);
        }

        clicks_re = std::regex(R"(Right clicks:\s*(\d+))");
        if (std::regex_search(response, m, clicks_re))
        {
            status.right_clicks = std::stoull(m[1]);
        }

        clicks_re = std::regex(R"(Middle clicks:\s*(\d+))");
        if (std::regex_search(response, m, clicks_re))
        {
            status.middle_clicks = std::stoull(m[1]);
        }

        clicks_re = std::regex(R"(Backward clicks:\s*(\d+))");
        if (std::regex_search(response, m, clicks_re))
        {
            status.backward_clicks = std::stoull(m[1]);
        }

        clicks_re = std::regex(R"(Forward clicks:\s*(\d+))");
        if (std::regex_search(response, m, clicks_re))
        {
            status.forward_clicks = std::stoull(m[1]);
        }

        clicks_re = std::regex(R"(Downward scrolls:\s*(\d+))");
        if (std::regex_search(response, m, clicks_re))
        {
            status.downward_scrolls = std::stoull(m[1]);
        }

        clicks_re = std::regex(R"(Upward scrolls:\s*(\d+))");
        if (std::regex_search(response, m, clicks_re))
        {
            status.upward_scrolls = std::stoull(m[1]);
        }

        std::regex battery_re(R"(Battery level:\s*(-?\d+)mV\s*=\s*(\d+)%?)");
        if (std::regex_search(response, m, battery_re))
        {
            status.battery_mv = std::stoi(m[1]);
            status.battery_percent = std::stoi(m[2]);
        }

        std::regex dpi_re(R"(Current DPI \[.*\]:\s*(\d+))");
        if (std::regex_search(response, m, dpi_re))
        {
            status.current_dpi = std::stoi(m[1]);
        }

        return true;
    }

    bool set_device(Subject &subject)
    {
        if (hSerial == INVALID_HANDLE_VALUE)
        {
            std::cerr << "[app_scan_and_detect] Invalid handle. Probably disconnected.\n";
            return false;
        }

        std::cout << "[app_scan_and_detect] Sending ENTER character to probe device...\n";

        const char enter = '\n';
        DWORD bw = 0;
        if (!WriteFile(hSerial, &enter, 1, &bw, NULL))
        {
            std::cerr << "[app_scan_and_detect] Write failed. Device may be disconnected.\n";
            return false;
        }

        char buf[2000] = {0};
        DWORD br = 0;
        if (!ReadFile(hSerial, buf, sizeof(buf) - 1, &br, NULL))
        {
            std::cerr << "[app_scan_and_detect] Read failed. Device may be disconnected or not responding.\n";
            return false;
        }

        std::string line(buf, br);
        std::cout << "[app_scan_and_detect] Received response: \"" << line << "\"\n";

        std::smatch match;
        std::regex pattern("-+ (\\w+) COM PORT -+");

        if (std::regex_search(line, match, pattern))
        {
            std::string subj = match[1];
            std::cout << "[app_scan_and_detect] Matched pattern. Subject detected: " << subj << "\n";

            if (subj == "MOUSE")
            {
                subject = Subject::MOUSE;
                read_func = read_mouse_data;
                std::cout << "[app_scan_and_detect] Handler set to read_data_mouse.\n";
            }
            else if (subj == "RECEIVER")
            {
                subject = Subject::RECEIVER;
                // read_func = read_data_receiver;
                std::cout << "[app_scan_and_detect] Handler set to read_data_receiver.\n";
            }
            else
            {
                std::cerr << "[app_scan_and_detect] Unknown subject type: " << subj << "\n";
                subject = Subject::UNKNOWN;
            }
            return true;
        }
        else
        {
            std::cout << "[app_scan_and_detect] Pattern not found in response. Will try again.\n";
        }

        return true; // keep looping, subject still UNKNOWN
    }

    void print_status(MouseStatus &status)
    {
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
}
