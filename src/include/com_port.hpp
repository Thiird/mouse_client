#pragma once
#include <string>
#include <functional>
#include <inttypes.h>

#include "../include/com_port.hpp"
#include <windows.h>
#include <iostream>
#include <regex>
#include <sstream>
#include <expected>

namespace ComPort
{
    enum class Subject
    {
        UNKNOWN,
        MOUSE,
        RECEIVER
    };

    enum class Platform
    {
        UNKNOWN,
        WINDOWS,
        LINUX
    };

    struct MouseStatus
    {
        std::string firmware_build_date;
        uint64_t left_clicks = 0;
        uint64_t right_clicks = 0;
        uint64_t middle_clicks = 0;
        uint64_t backward_clicks = 0;
        uint64_t forward_clicks = 0;
        uint64_t downward_scrolls = 0;
        uint64_t upward_scrolls = 0;
        int battery_mv = 0;
        int battery_percent = 0;
        int current_dpi = 0;
    };

    static HANDLE hSerial = INVALID_HANDLE_VALUE;
    extern Subject connectedTo;
    extern bool (*read_data_X)(MouseStatus &status);
    static bool ret = false;

    // Static VID/PID for RECEIVER and MOUSE
    static const std::wstring vidPid = L"VID_2FE3&PID_0003&REV_0303";
    static const std::wstring receiverDescription = L"Cool mouse receiver";
    static const std::wstring mouseDescription = L"Cool mouse";

    bool detectDevices(std::wstring& mouseComPort, std::wstring& receiverComPort);
    bool connect(Subject targetSubject, std::wstring comPortName);
    void disconnect();
    bool set_device(Subject &subject);
    bool read_data_mouse(MouseStatus &status);
    bool read_data_receiver(MouseStatus &status);
    void print_status(MouseStatus &status);
}