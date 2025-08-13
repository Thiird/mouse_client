#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "include/gui.hpp"
#include "include/com_port.hpp"

std::atomic<bool> stopRequested(false);
ComPort::MouseStatus status;
bool connected = false;
// Detect available devices
std::wstring mouseComPort, receiverComPort;

// connect to MOUSE (preferred) or RECEIVER
bool selectDevice()
{
    if(ComPort::connectedTo == ComPort::Subject::MOUSE)
        return true;

    // Prioritize MOUSE if available, otherwise fall back to RECEIVER
    if (!mouseComPort.empty() && ComPort::connectedTo != ComPort::Subject::MOUSE)
    {
        ComPort::disconnect();

        std::cout << "MOUSE detected on " << std::string(mouseComPort.begin(), mouseComPort.end()) << ", attempting to connect" << std::endl;
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

    if (!receiverComPort.empty() && ComPort::connectedTo != ComPort::Subject::RECEIVER)
    {
        ComPort::disconnect();

        std::cout << "RECEIVER detected on " << std::string(receiverComPort.begin(), receiverComPort.end()) << ", attempting to connect" << std::endl;
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

    if(ComPort::connectedTo != ComPort::Subject::UNKNOWN)
    {
        return true;
    }

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
            wait_time = 1;
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
            wait_time = 1;
            Gui::updateGui(status, false);
            continue;
        }

        wait_time = 3;

        Gui::updateGui(status, true);       
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QAction *quitAction = nullptr;
    gui_init(app, &quitAction);

    std::thread monitoringThread([&]
                                 { startMonitoring(); });

    QObject::connect(quitAction, &QAction::triggered, [&]()
                     {
        stopRequested = true;
        monitoringThread.join();
        app.quit();
        std::cout << "Closing down" << std::endl; });

    return app.exec();
}