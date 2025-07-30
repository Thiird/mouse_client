#ifndef GUI_HPP
#define GUI_HPP

#include <QApplication>
#include <QSystemTrayIcon>
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>

void gui_init(QApplication& app, QMainWindow& mainWindow, QLabel*& dataLabel, QPushButton*& reloadButton, QSystemTrayIcon& trayIcon);
void gui_update_data(QLabel* dataLabel, const QString& text, bool isConnected, QMainWindow* mainWindow, QSystemTrayIcon* trayIcon);

#endif // GUI_H