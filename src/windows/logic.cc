#include "../include/logic.hpp"
#include <cstdlib>

Subject currentSubject = NONE;

bool read_mouse_data(MouseData& data) {
    static int counter = 0;
    // Simulate occasional connection failure
    if (rand() % 5 == 0) { // 20% chance of failure
        return false;
    }
    counter++;
    data = MouseData{
        100 + counter, 200 + counter, 50, 3, 1, 30, 20, 95, 700
    };
    return true;
}

QString format_mouse_data(const MouseData& data) {
    return QString(
        "Left clicks: %1\nRight clicks: %2\nMiddle clicks: %3\n"
        "Backward clicks: %4\nForward clicks: %5\n"
        "Down scrolls: %6\nUp scrolls: %7\n"
        "Battery level: %8%%\nDPI: %9"
    ).arg(data.leftClicks).arg(data.rightClicks).arg(data.middleClicks)
     .arg(data.backwardClicks).arg(data.forwardClicks)
     .arg(data.downScrolls).arg(data.upScrolls)
     .arg(data.batteryLevel).arg(data.dpi);
}

void logic_loop_init() {
    // Initialize any logic-related resources (e.g., COM port setup)
    // Placeholder for Windows-specific initialization
}