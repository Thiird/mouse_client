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
    static Subject currentSubject = Subject::UNKNOWN;
    static bool (*read_func)(MouseStatus &status) = nullptr;

    bool connect();

    void disconnect();

    bool set_device(Subject &subject);

    bool read_mouse_data(MouseStatus &status);

    void print_status(MouseStatus &status);

}
