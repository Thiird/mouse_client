#ifndef LOGIC_H
#define LOGIC_H

#include <QString>

enum Subject { NONE, MOUSE, RECEIVER };

struct MouseData {
    int leftClicks = 0;
    int rightClicks = 0;
    int middleClicks = 0;
    int backwardClicks = 0;
    int forwardClicks = 0;
    int downScrolls = 0;
    int upScrolls = 0;
    int batteryLevel = 0;
    int dpi = 0;
};

void logic_loop_init();
bool read_mouse_data(MouseData& data);
QString format_mouse_data(const MouseData& data);

#endif // LOGIC_H