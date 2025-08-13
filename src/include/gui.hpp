// gui.hpp
#pragma once
#include <QMainWindow>
#include <QApplication>
#include <QObject>
#include "../include/com_port.hpp" // adjust include if needed

class Gui : public QObject
{
public:
    Gui(QApplication &app, QObject *parent = nullptr);
    ~Gui();
    static void updateGui(ComPort::MouseStatus &data, bool connected);
    static bool guiOpen;

private:
    QApplication &app;
};

// NEW: subclass QMainWindow to override closeEvent
class MainWindow : public QMainWindow
{
public:
    using QMainWindow::QMainWindow; // inherit constructors

protected:
    void closeEvent(QCloseEvent *event) override;
};

void gui_init(QApplication &app, QAction **quitActionOut);

QString format_mouse_data(const ComPort::MouseStatus &data);

