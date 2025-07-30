#include "../include/gui.hpp"
#include <QMenu>
#include <QAction>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QIcon>
#include <QDateTime>

#define APP_VERSION "0.1"

void gui_init(QApplication& app, QMainWindow& mainWindow, QLabel*& dataLabel, QPushButton*& reloadButton, QSystemTrayIcon& trayIcon) {
    app.setQuitOnLastWindowClosed(false);

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        exit(1);
    }

    // Main window
    mainWindow.setWindowTitle("Not connected");

    // Top bar menu
    QMenu* fileMenu = new QMenu("File");
    QAction* saveAction = new QAction("Save to file");
    fileMenu->addAction(saveAction);
    QMenu* aboutMenu = new QMenu("About");
    QAction* aboutAction = new QAction("About");
    aboutMenu->addAction(aboutAction);
    QMenuBar* menuBar = new QMenuBar();
    menuBar->addMenu(fileMenu);
    menuBar->addMenu(aboutMenu);
    mainWindow.setMenuBar(menuBar);

    // Central widget: one tab with mouse data
    QWidget* centralWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout();
    dataLabel = new QLabel("No data yet");
    reloadButton = new QPushButton();
    reloadButton->setIcon(QIcon::fromTheme("view-refresh"));
    reloadButton->setToolTip("Reload data");
    reloadButton->setEnabled(false); // Initially disabled

    layout->addWidget(dataLabel);
    layout->addWidget(reloadButton);
    centralWidget->setLayout(layout);
    mainWindow.setCentralWidget(centralWidget);

    // System tray icon & menu
    QIcon connectedIcon("res/icons/connected.png");
    QIcon disconnectedIcon("res/icons/disconnected.png");
    trayIcon.setIcon(disconnectedIcon);
    trayIcon.setToolTip("Not connected");

    // Dynamically allocate the tray menu to ensure it persists
    QMenu* trayMenu = new QMenu(&mainWindow); // Parent to mainWindow for proper cleanup
    QAction* openAction = new QAction("Open");
    QAction* quitAction = new QAction("Quit");
    trayMenu->addAction(openAction);
    trayMenu->addAction(quitAction);
    trayIcon.setContextMenu(trayMenu);
    trayIcon.show();

    // About action shows version & compile date
    QObject::connect(aboutAction, &QAction::triggered, [&]() {
        QString msg = QString("Version: %1\nCompiled on: %2 %3")
                .arg(APP_VERSION)
                .arg(__DATE__)
                .arg(__TIME__);
        QMessageBox::about(&mainWindow, "About", msg);
    });

    // Open action shows window
    QObject::connect(openAction, &QAction::triggered, [&]() {
        mainWindow.show();
        mainWindow.raise();
        mainWindow.activateWindow();
    });
    QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);

    trayIcon.showMessage("Tray App", "App started and running in tray.", QSystemTrayIcon::Information, 3000);
}

void gui_update_data(QLabel* dataLabel, const QString& text, bool isConnected, QMainWindow* mainWindow, QSystemTrayIcon* trayIcon) {
    QIcon connectedIcon = QIcon::fromTheme("network-transmit-receive");
    QIcon disconnectedIcon = QIcon::fromTheme("network-offline");

    if (isConnected) {
        mainWindow->setWindowTitle("Connected to MOUSE");
        trayIcon->setIcon(connectedIcon);
        trayIcon->setToolTip("Connected to MOUSE");
        dataLabel->setText(text);
    } else {
        mainWindow->setWindowTitle("Not connected");
        trayIcon->setIcon(disconnectedIcon);
        trayIcon->setToolTip("Not connected");
        dataLabel->setText("No data - not connected");
    }
}