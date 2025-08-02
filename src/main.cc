#include "include/gui.hpp"
#include <thread>
#include <QDebug>

ComPort::MouseStatus status;
bool connected = false;

void startMonitoring()
{
    appendLogMessage("AAA", Qt::green);
    while (true)
    {
        while (!ComPort::connect())
        {
            appendLogMessage("Trying to connect to com port..", Qt::red);
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }

        appendLogMessage("Connected", Qt::green);
        connected = true;


        while (connected)
        {
            
        Gui::updateGui(status, connected);
            if (ComPort::read_mouse_data(status))
            {
                ComPort::print_status(status);

                appendLogMessage("Read data", Qt::black);
            }
            else
            {
                appendLogMessage("Could not read mouse data...", Qt::black);
                break;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        Gui::updateGui(status, connected);
        connected = false;

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Initialize GUI
    QApplication &guiApp = gui_init(app);

    std::thread monitoringThread([&]
                                 { startMonitoring(); });

    // Run the application loop
    return guiApp.exec();
}