#include <QApplication>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QLabel>
#include <QPushButton>
#include "./include/logic.hpp"
#include "./include/gui.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QMainWindow mainWindow;
    QSystemTrayIcon trayIcon;
    QLabel* dataLabel = nullptr;
    QPushButton* reloadButton = nullptr;

    // Initialize logic
    logic_loop_init();

    // Initialize GUI
    gui_init(app, mainWindow, dataLabel, reloadButton, trayIcon);

    // Update connection and data
    auto update_connection = [&]() {
        MouseData data;
        bool isConnected = read_mouse_data(data);
        QString text = isConnected ? format_mouse_data(data) : "";
        gui_update_data(dataLabel, text, isConnected, &mainWindow, &trayIcon);
        reloadButton->setEnabled(isConnected);
    };

    // Connect reload button
    QObject::connect(reloadButton, &QPushButton::clicked, update_connection);

    // Initial connection check
    update_connection();

    // Run application loop
    return app.exec();
}