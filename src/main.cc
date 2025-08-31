#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <string>

#include <QApplication>
#include <QAction>

#include "include/gui.hpp"
#include "include/com_port.hpp"

// -------------------- Globals --------------------
std::atomic<bool> stopRequested(false);
ComPort::MouseStatus status;
bool connected = false;

// Detected COM ports
std::wstring mouseComPort, receiverComPort;

// -------------------- Functions --------------------
bool selectDevice()
{
    if (ComPort::connectedTo == ComPort::Subject::MOUSE)
        return true;

    // Prioritize MOUSE
    if (!mouseComPort.empty() && ComPort::connectedTo != ComPort::Subject::MOUSE)
    {
        ComPort::disconnect();
        std::cout << "MOUSE detected on "
                  << std::string(mouseComPort.begin(), mouseComPort.end())
                  << ", attempting to connect" << std::endl;

        if (ComPort::connect(ComPort::Subject::MOUSE, mouseComPort))
        {
            std::cout << "Connected to MOUSE" << std::endl;
            ComPort::connectedTo = ComPort::Subject::MOUSE;
            return true;
        }
        else
        {
            std::cout << "Failed to connect to MOUSE" << std::endl;
        }
    }

    // Fallback to RECEIVER
    if (!receiverComPort.empty() && ComPort::connectedTo != ComPort::Subject::RECEIVER)
    {
        ComPort::disconnect();
        std::cout << "RECEIVER detected on "
                  << std::string(receiverComPort.begin(), receiverComPort.end())
                  << ", attempting to connect" << std::endl;

        if (ComPort::connect(ComPort::Subject::RECEIVER, receiverComPort))
        {
            std::cout << "Connected to RECEIVER" << std::endl;
            ComPort::connectedTo = ComPort::Subject::RECEIVER;
            return true;
        }
        else
        {
            std::cout << "Failed to connect to RECEIVER" << std::endl;
        }
    }

    if (ComPort::connectedTo != ComPort::Subject::UNKNOWN)
        return true;

    std::cout << "No valid connection established" << std::endl;
    return false;
}

void startMonitoring()
{
    int wait_time = 1;

    while (!stopRequested)
    {
        std::this_thread::sleep_for(std::chrono::seconds(wait_time));
        wait_time = 60;

        if (stopRequested)
            break;

        ComPort::detectDevices(mouseComPort, receiverComPort);

        if (!selectDevice())
        {
            std::cout << "Trying to connect to COM port..." << std::endl;
            wait_time = 10;
            Gui::updateGui(status, false);
            continue;
        }

        if (!ComPort::read_data_X)
        {
            std::cout << "No read data function" << std::endl;
            continue;
        }

        if (!ComPort::read_data_X(status))
        {
            std::cout << "Could not read data, disconnecting" << std::endl;
            ComPort::disconnect();
            wait_time = 10;
            Gui::updateGui(status, false);
            continue;
        }

        Gui::updateGui(status, true);
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QAction *quitAction = nullptr;
    gui_init(app, &quitAction);

    // Start monitoring in a separate thread
    std::thread monitoringThread(startMonitoring);

    // Connect the quit action to stop thread and quit
    QObject::connect(quitAction, &QAction::triggered, [&]()
                     {
                         stopRequested = true;
                         if (monitoringThread.joinable())
                             monitoringThread.join();
                         app.quit();
                         std::cout << "Closing down" << std::endl;
                     });

    return app.exec();
}

#ifdef _WIN32
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    // Convert lpCmdLine to argc/argv
    int argc = 0;
    char* argv[] = { nullptr };
    return main(argc, argv);
}
#endif
