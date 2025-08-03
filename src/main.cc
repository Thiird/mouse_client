#include "include/gui.hpp"
#include <thread>
#include <QDebug>
#include <atomic>

std::atomic<bool> stopRequested(false);

ComPort::MouseStatus status;
bool connected = false;

void startMonitoring()
{
    while (!stopRequested)
    {
        while (!ComPort::connect() && !stopRequested)
        {
            std::cout <<"Trying to connect to com port.." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        if (stopRequested) break;

        std::cout <<"Connected" << std::endl;
        connected = true;

        while (connected && !stopRequested)
        {
            if (ComPort::read_mouse_data(status))
            {
                Gui::updateGui(status, connected);
                std::cout << "Read data" << std::endl;
            }
            else
            {
                std::cout << "Could not read mouse data" << std::endl;
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        connected = false;
        Gui::updateGui(status, connected);

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QAction* quitAction = nullptr;
    gui_init(app, &quitAction);

    std::thread monitoringThread([&] { startMonitoring(); });

    QObject::connect(quitAction, &QAction::triggered, [&]()
    {
        stopRequested = true;
        monitoringThread.join();
        app.quit();
        std::cout << "Closing down" << std::endl;
    });

    return app.exec();
}