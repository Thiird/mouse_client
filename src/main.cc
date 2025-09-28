#include <atomic>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <windows.h>
#include <dbt.h>

#include <QApplication>
#include <QAction>

#include "include/gui.hpp"
#include "include/com_port.hpp"

// -------------------- Globals --------------------
std::atomic<bool> stopRequested(false);
std::atomic<bool> deviceConnected(false);
std::atomic<int> comPortEvents(1); // initial scan triggers first run
ComPort::MouseStatus status;

std::wstring mouseComPort, receiverComPort;
HWND comNotifyHwnd = nullptr;

// -------------------- Functions --------------------
bool selectDevice()
{
    ComPort::connectedTo = ComPort::Subject::NONE;

    if (!mouseComPort.empty())
    {
        ComPort::disconnect();
        std::cout << "MOUSE detected on "
                  << std::string(mouseComPort.begin(), mouseComPort.end())
                  << ", attempting to connect" << std::endl;

        if (ComPort::connect(ComPort::Subject::MOUSE, mouseComPort))
        {
            std::cout << "Connected to MOUSE" << std::endl;
            ComPort::connectedTo = ComPort::Subject::MOUSE;
            deviceConnected = true;
            return true;
        }
    }

    if (!receiverComPort.empty())
    {
        ComPort::disconnect();
        std::cout << "RECEIVER detected on "
                  << std::string(receiverComPort.begin(), receiverComPort.end())
                  << ", attempting to connect" << std::endl;

        if (ComPort::connect(ComPort::Subject::RECEIVER, receiverComPort))
        {
            std::cout << "Connected to RECEIVER" << std::endl;
            ComPort::connectedTo = ComPort::Subject::RECEIVER;
            deviceConnected = true;
            return true;
        }
    }

    deviceConnected = false;
    return false;
}

// -------------------- Device monitoring thread --------------------
void deviceMonitoringThread()
{
    while (!stopRequested)
    {
        if (comPortEvents == 0)
        {
            Sleep(500);
            continue;
        }

        comPortEvents--; // consume event

        if (!ComPort::detectDevices(mouseComPort, receiverComPort))
        {
            std::cout << "Could not detect any device" << std::endl;
            continue;
        }

        if (!selectDevice())
        {
            std::cout << "Could not select a COM port..." << std::endl;
            Gui::updateGui(status, false);
        }

        std::cout << "Selected a device" << std::endl;
    }
}

// -------------------- Data reading thread --------------------
void dataReadingThread()
{
    while (!stopRequested)
    {
        if (!deviceConnected)
        {
            Sleep(1000);
            continue;
        }

        if (!ComPort::read_data_X)
        {
            std::cout << "No read data function" << std::endl;
            Sleep(1000);
            continue;
        }

        if (!ComPort::read_data_X(status))
        {
            std::cout << "Could not read data, disconnecting" << std::endl;
            ComPort::disconnect();
            deviceConnected = false;
            Gui::updateGui(status, false);
            ComPort::connectedTo = ComPort::Subject::NONE;
            Sleep(2000);
            continue;
        }

        Gui::updateGui(status, true);
        Sleep(2000);
    }
}

// -------------------- COM port notification thread --------------------
void comPortNotificationThread()
{
    const wchar_t CLASS_NAME[] = L"ComPortNotifyWindow";
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASS wc{};
    wc.lpfnWndProc = [](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT
    {
        switch (uMsg)
        {
        case WM_DEVICECHANGE:
            if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE)
            {
                PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
                if (pHdr && pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
                {
                    std::cout << "Something com port happened" << std::endl;
                    deviceConnected = false;
                    comPortEvents++;
                }
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    };

    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClass(&wc))
    {
        std::cerr << "Failed to register window class" << std::endl;
        return;
    }

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"COM Port Notification Window",
        0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, hInstance, nullptr);

    comNotifyHwnd = hwnd;

    if (!hwnd)
    {
        std::cerr << "Failed to create window" << std::endl;
        return;
    }

    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter{};
    NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_COMPORT;

    HDEVNOTIFY hDevNotify = RegisterDeviceNotification(
        hwnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);

    if (!hDevNotify)
    {
        std::cerr << "Failed to register device notification" << std::endl;
        return;
    }

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnregisterDeviceNotification(hDevNotify);
}

#ifdef _WIN32
#include <windows.h>
#endif

// -------------------- Main --------------------
int main(int argc, char *argv[])
{

#ifdef _WIN32

    bool noConsole = false;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--no-console") == 0)
        {
            noConsole = true;
            break;
        }
    }
    if (noConsole)
    {
        FreeConsole();
    }
#endif

    QApplication app(argc, argv);

    QAction *quitAction = nullptr;
    gui_init(app, &quitAction);

    std::thread comNotifyThread(comPortNotificationThread);
    std::thread monitorThread(deviceMonitoringThread);
    std::thread readerThread(dataReadingThread);

    QObject::connect(quitAction, &QAction::triggered, [&]()
                     {
        stopRequested = true;

        if (comNotifyHwnd)
            PostMessage(comNotifyHwnd, WM_QUIT, 0, 0);

        if (monitorThread.joinable()) monitorThread.join();
        if (readerThread.joinable()) readerThread.join();
        if (comNotifyThread.joinable()) comNotifyThread.join();

        app.quit(); });

    return app.exec();
}