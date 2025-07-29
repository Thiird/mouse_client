#include "./include/com_port.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    Platform plat = get_current_platform();
    std::cout << "Running on platform: "
              << (plat==Platform::WINDOWS ? "Windows" :
                  plat==Platform::LINUX ? "Linux" : "Unknown") << "\n";

    while (true) {
        if (!app_init()) {
            std::cerr << "Init failed. Retrying...\n";
            continue;
        }

        Subject subject = Subject::UNKNOWN;

        // Keep scanning until subject detected
        while (subject == Subject::UNKNOWN) {
            if (!app_scan_and_detect(subject)) {
                std::cerr << "Failed to scan. Reconnecting...\n";
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        if (subject != Subject::UNKNOWN) {
            std::cout << "Subject found, starting periodic read every minute...\n";

            while (true) {
                DataReadFunc func = get_current_read_func();
                if (func) {
                    func();
                } else {
                    std::cerr << "No handler set. Reconnecting...\n";
                    break;
                }
                std::this_thread::sleep_for(std::chrono::minutes(1));
            }
        }

        app_close();
        std::cout << "Disconnected, will retry...\n";
    }

    return 0;
}
