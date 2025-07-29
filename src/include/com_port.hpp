#pragma once
#include <string>
#include <functional>

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
