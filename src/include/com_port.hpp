#pragma once
#include <string>
#include <functional>
#include <inttypes.h>

enum class Subject {
    UNKNOWN,
    MOUSE,
    RECEIVER
};

enum class Platform {
    UNKNOWN,
    WINDOWS,
    LINUX
};

struct MouseStatus {
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

// Initialize (try connect, detect subject)
bool app_init();

// Close resources
void app_close();

// Write ENTER, read line, detect subject if not detected yet
bool app_scan_and_detect(Subject &subject);

// Once subject detected: set function pointer to correct handler
using DataReadFunc = void(*)();

// Example handlers to set
void read_data_mouse();
void read_data_receiver();

// Get detected platform at startup
Platform get_current_platform();

extern DataReadFunc get_current_read_func();
