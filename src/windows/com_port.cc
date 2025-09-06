#include <windows.h>
#include <setupapi.h>
#include <iostream>
#include <string>
#include <regex>

#include <QApplication>
#include <QDateTime>

#include "../include/com_port.hpp"
#include "../include/gui.hpp"

#pragma comment(lib, "setupapi.lib")

namespace ComPort
{
    bool (*read_data_X)(MouseStatus &status) = nullptr;
    Subject connectedTo = Subject::UNKNOWN;

    bool detectDevices(std::wstring &mouseComPort, std::wstring &receiverComPort)
    {
        mouseComPort.clear();
        receiverComPort.clear();

        HDEVINFO deviceInfoSet = SetupDiGetClassDevs(NULL, L"USB", NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES);
        if (deviceInfoSet == INVALID_HANDLE_VALUE)
        {
            std::cerr << "SetupDiGetClassDevs failed" << std::endl;
            return false;
        }

        SP_DEVINFO_DATA deviceInfoData = {sizeof(SP_DEVINFO_DATA)};
        for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++)
        {
            WCHAR hardwareId[256] = {0};
            if (SetupDiGetDeviceRegistryProperty(deviceInfoSet, &deviceInfoData, SPDRP_HARDWAREID, NULL,
                                                 (BYTE *)hardwareId, sizeof(hardwareId), NULL))
            {
                std::wstring hwId(hardwareId);

                WCHAR portName[256] = {0};
                DWORD size = sizeof(portName);
                HKEY hKey = SetupDiOpenDevRegKey(deviceInfoSet, &deviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
                if (hKey != INVALID_HANDLE_VALUE)
                {
                    if (RegQueryValueExW(hKey, L"PortName", NULL, NULL, (BYTE *)portName, &size) == ERROR_SUCCESS)
                    {
                        std::wstring port(portName);
                        if (hwId.find(mouseVidPid) != std::wstring::npos)
                            mouseComPort = port;
                        else if (hwId.find(receiverVidPid) != std::wstring::npos)
                            receiverComPort = port;
                    }
                    RegCloseKey(hKey);
                }
            }
        }

        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return !mouseComPort.empty() || !receiverComPort.empty();
    }

    bool connect(Subject targetSubject, std::wstring targetComPort)
    {
        if (targetComPort.empty())
        {
            std::cerr << "No COM port found for " 
                      << (targetSubject == Subject::MOUSE ? "MOUSE" : "RECEIVER") 
                      << std::endl;
            return false;
        }

        std::wstring comPath = L"\\\\.\\" + targetComPort;
        hSerial = CreateFileW(comPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hSerial == INVALID_HANDLE_VALUE)
        {
            std::cerr << "Failed to connect to " << std::string(targetComPort.begin(), targetComPort.end()) << std::endl;
            return false;
        }

        std::cout << "Connected to " << std::string(targetComPort.begin(), targetComPort.end()) << std::endl;

        DCB dcb = {0};
        dcb.DCBlength = sizeof(dcb);
        ret = GetCommState(hSerial, &dcb);
        if (!ret)
        {
            std::cerr << "GetCommState failed" << std::endl;
            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;
            return false;
        }

        dcb.BaudRate = CBR_115200;
        dcb.ByteSize = 8;
        dcb.StopBits = ONESTOPBIT;
        dcb.Parity   = NOPARITY;
        ret = SetCommState(hSerial, &dcb);
        if (!ret)
        {
            std::cerr << "SetCommState failed" << std::endl;
            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;
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
            std::cerr << "SetCommTimeouts failed" << std::endl;
            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;
            return false;
        }

        connectedTo = targetSubject;
        read_data_X = (connectedTo == Subject::MOUSE) ? read_data_mouse : read_data_receiver;
        return true;
    }

    void disconnect()
    {
        if (hSerial != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;
        }
        connectedTo = Subject::UNKNOWN;
        read_data_X = nullptr;
    }

    bool read_data_receiver(MouseStatus &status)
    {
        const char enter = '1';
        DWORD bw = 0;
        if (!WriteFile(hSerial, &enter, 1, &bw, NULL))
        {
            std::cout << "Failed to send ENTER" << std::endl;
            return false;
        }

        char buf[1024] = {0};
        DWORD br = 0;
        if (!ReadFile(hSerial, buf, sizeof(buf) - 1, &br, NULL))
            return false;

        std::string response(buf, br);
        std::smatch m;
        std::regex battery_re(R"(Battery level:\s*(-?\d+)mV\s*=\s*(\d+)%?)");
        if (std::regex_search(response, m, battery_re))
        {
            status.battery_mv      = std::stoi(m[1]);
            status.battery_percent = std::stoi(m[2]);
        }

        Gui::lastReadingTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        return true;
    }

    bool read_data_mouse(MouseStatus &status)
    {
        const char enter = '1';
        DWORD bw = 0;
        if (!WriteFile(hSerial, &enter, 1, &bw, NULL))
        {
            std::cout << "Failed to send ENTER" << std::endl;
            return false;
        }

        char buf[1024] = {0};
        DWORD br = 0;
        if (!ReadFile(hSerial, buf, sizeof(buf) - 1, &br, NULL))
            return false;

        std::string response(buf, br);
        std::smatch m;

        std::regex clicks_re(R"(Left clicks:\s*(\d+))");
        if (std::regex_search(response, m, clicks_re))
            status.left_clicks = std::stoull(m[1]);

        clicks_re = std::regex(R"(Right clicks:\s*(\d+))");
        if (std::regex_search(response, m, clicks_re))
            status.right_clicks = std::stoull(m[1]);

        clicks_re = std::regex(R"(Middle clicks:\s*(\d+))");
        if (std::regex_search(response, m, clicks_re))
            status.middle_clicks = std::stoull(m[1]);

        clicks_re = std::regex(R"(Backward clicks:\s*(\d+))");
        if (std::regex_search(response, m, clicks_re))
            status.backward_clicks = std::stoull(m[1]);

        clicks_re = std::regex(R"(Forward clicks:\s*(\d+))");
        if (std::regex_search(response, m, clicks_re))
            status.forward_clicks = std::stoull(m[1]);

        clicks_re = std::regex(R"(Downward scrolls:\s*(\d+))");
        if (std::regex_search(response, m, clicks_re))
            status.downward_scrolls = std::stoull(m[1]);

        clicks_re = std::regex(R"(Upward scrolls:\s*(\d+))");
        if (std::regex_search(response, m, clicks_re))
            status.upward_scrolls = std::stoull(m[1]);

        std::regex battery_re(R"(Battery level:\s*(-?\d+)mV\s*=\s*(\d+)%?)");
        if (std::regex_search(response, m, battery_re))
        {
            status.battery_mv      = std::stoi(m[1]);
            status.battery_percent = std::stoi(m[2]);
        }

        std::regex dpi_re(R"(Current DPI \[.*\]:\s*(\d+))");
        if (std::regex_search(response, m, dpi_re))
            status.current_dpi = std::stoi(m[1]);

        Gui::lastReadingTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        return true;
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

    // Simplified helper to select and connect to device
    bool autoConnect(MouseStatus &status)
    {
        std::wstring mousePort, receiverPort;
        detectDevices(mousePort, receiverPort);

        if (!mousePort.empty())
            return connect(Subject::MOUSE, mousePort);
        else if (!receiverPort.empty())
            return connect(Subject::RECEIVER, receiverPort);

        return false;
    }
}
