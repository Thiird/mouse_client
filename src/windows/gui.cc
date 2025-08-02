#include "../include/gui.hpp"
#include <QMenu>
#include <QAction>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QIcon>
#include <QDateTime>
#include <iostream>
#include <QDebug>
#include <QSystemTrayIcon>
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QApplication>

#define APP_VERSION "0.1"

// Static pointers for dynamic objects
QMainWindow *mainWindow = nullptr;
QSystemTrayIcon *trayIcon = nullptr;
QLabel *dataLabel = nullptr;
QPushButton *reloadButton = nullptr;
QIcon *connectedIcon = nullptr;
QIcon *disconnectedIcon = nullptr;
QTextEdit *logBox = nullptr;

// Gui implementation
Gui::Gui(QApplication &app, QObject *parent) : QObject(parent), app(app) {}
Gui::~Gui() {}

QString format_mouse_data(const ComPort::MouseStatus &data)
{
    return QString(
               "Left clicks: %1\nRight clicks: %2\nMiddle clicks: %3\n"
               "Backward clicks: %4\nForward clicks: %5\n"
               "Down scrolls: %6\nUp scrolls: %7\n"
               "Battery level: %8%%\nDPI: %9")
        .arg(data.left_clicks)
        .arg(data.right_clicks)
        .arg(data.middle_clicks)
        .arg(data.backward_clicks)
        .arg(data.forward_clicks)
        .arg(data.downward_scrolls)
        .arg(data.upward_scrolls)
        .arg(data.battery_percent)
        .arg(data.current_dpi);
}

void Gui::updateGui(ComPort::MouseStatus &data, bool connected)
{
     if (connected) {
         mainWindow->setWindowTitle("Connected to MOUSE");
         trayIcon->setIcon(*connectedIcon);
         trayIcon->setToolTip("Connected to MOUSE");
         dataLabel->setText(format_mouse_data(data));
     } else {
         mainWindow->setWindowTitle("Not connected");
         trayIcon->setIcon(*disconnectedIcon);
         trayIcon->setToolTip("Not connected");
         dataLabel->setText("No data - not connected");
     }
     reloadButton->setEnabled(connected);
}

QApplication &gui_init(QApplication &app)
{
    // Initialize QIcons after QApplication is created
    if (!connectedIcon)
        connectedIcon = new QIcon(":res/icons/mouse.png");
    if (!disconnectedIcon)
        disconnectedIcon = new QIcon(":res/icons/mouse_not.png");
    if (connectedIcon->isNull() || disconnectedIcon->isNull())
    {
        qDebug() << "One or more icons failed to load!";
        exit(1);
    }

    // Set application icon
    app.setWindowIcon(*connectedIcon);

    // Initialize main window
    mainWindow = new QMainWindow();
    mainWindow->setWindowTitle("Not connected");
    mainWindow->setWindowIcon(*connectedIcon);

    // Top bar menu
    QMenu *fileMenu = new QMenu("File");
    QAction *saveAction = new QAction("Save to file");
    fileMenu->addAction(saveAction);
    QMenu *aboutMenu = new QMenu("About");
    QAction *aboutAction = new QAction("About");
    aboutMenu->addAction(aboutAction);
    QMenuBar *menuBar = new QMenuBar();
    menuBar->addMenu(fileMenu);
    menuBar->addMenu(aboutMenu);
    mainWindow->setMenuBar(menuBar);

    // Central widget: one tab with mouse data
    QWidget *centralWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    dataLabel = new QLabel("No data yet");
    reloadButton = new QPushButton();
    reloadButton->setIcon(QIcon::fromTheme("view-refresh"));
    reloadButton->setToolTip("Reload data");
    reloadButton->setEnabled(false);

    layout->addWidget(dataLabel);
    layout->addWidget(reloadButton);

    logBox = new QTextEdit();
    logBox->setReadOnly(true);
    logBox->setFixedHeight(120); // adjust height if needed
    layout->addWidget(logBox);

    centralWidget->setLayout(layout);
    mainWindow->setCentralWidget(centralWidget);

    // System tray icon & menu
    trayIcon = new QSystemTrayIcon(mainWindow);
    trayIcon->setIcon(*disconnectedIcon);
    trayIcon->setToolTip("Not connected");

    QMenu *trayMenu = new QMenu(mainWindow);
    QAction *openAction = new QAction("Open");
    QAction *quitAction = new QAction("Quit");
    trayMenu->addAction(openAction);
    trayMenu->addAction(quitAction);
    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();

    // About action
    QObject::connect(aboutAction, &QAction::triggered, [=]()
                     {
        QString msg = QString("Version: %1\nCompiled on: %2 %3")
                .arg(APP_VERSION)
                .arg(__DATE__)
                .arg(__TIME__);
        QMessageBox::about(mainWindow, "About", msg); });

    // Open action
    QObject::connect(openAction, &QAction::triggered, [=]()
                     {
        mainWindow->show();
        mainWindow->raise();
        mainWindow->activateWindow(); });

    // Quit action
    QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);

    // Tray message
    //trayIcon->showMessage("Tray App", "App started and running in tray.", QSystemTrayIcon::Information, 3000);

    // Set up GUI updater
    Gui *updater = new Gui(app, mainWindow);
    // QObject::connect(logicManager, &LogicManager::dataUpdated, updater, &Gui::updateGui);

    return app;
}

void appendLogMessage(const QString& message, const QColor& color) {
    if (!logBox) return;
    QString coloredMsg = QString("<span style='color:%1;'>%2</span>")
                         .arg(color.name(), message.toHtmlEscaped());
    logBox->append(coloredMsg);
}

