#pragma once

#include <QObject>
#include "com_port.hpp"

#include <QApplication>

// Forward declarations
class QMainWindow;
class QSystemTrayIcon;
class QLabel;
class QPushButton;
class QIcon;

// Global pointers (keep as is for now, consider replacing with a manager class later)
extern QMainWindow* mainWindow;
extern QSystemTrayIcon* trayIcon;
extern QLabel* dataLabel;
extern QPushButton* reloadButton;
extern QIcon* connectedIcon;
extern QIcon* disconnectedIcon;
#include <QTextEdit> // add this include

extern QTextEdit* logBox; // declare globally

class Gui : public QObject {
public:
    Gui(QApplication& app, QObject* parent = nullptr);
    ~Gui();

public slots:
    static void updateGui(ComPort::MouseStatus& data, bool connected);

private:
    QApplication& app;
};

QApplication& gui_init(QApplication& app);

void appendLogMessage(const QString& message, const QColor& color = Qt::black);
