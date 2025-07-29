#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        return 1;
    }

    QSystemTrayIcon trayIcon;
    trayIcon.setIcon(QIcon::fromTheme("applications-system")); // Use your own icon path here
    trayIcon.setToolTip("Hello from tray!");

    QMenu menu;
    QAction quitAction("Quit");
    QObject::connect(&quitAction, &QAction::triggered, &app, &QApplication::quit);
    menu.addAction(&quitAction);

    trayIcon.setContextMenu(&menu);
    trayIcon.show();

    trayIcon.showMessage("Hello", "Hello World from tray!", QSystemTrayIcon::Information, 3000);

    return app.exec();
}
