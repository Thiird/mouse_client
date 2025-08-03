#include "../include/gui.hpp"
#include <QMenu>
#include <QAction>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QIcon>
#include <QDateTime>
#include <QDebug>
#include <QSystemTrayIcon>
#include <QLabel>
#include <QPushButton>
#include <QApplication>
#include <QCloseEvent>
#include <QFile>
#include <QTextStream>
#include <QScrollArea>
#include <QDialog>
#include <QTextBrowser>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QAudioOutput>
#include <QMovie> // For GIF support

#define APP_VERSION "0.1"
#define WINDOW_SIZE_X 300  // Fixed width
#define WINDOW_SIZE_Y 300  // Fixed width
#define STATS_FILENAME "mouse_stats.txt"
#define THALES_FILENAME "mouse_thales.txt"
#define BEEP_FILENAME "beep.mp3"
#define GIF_FILENAME "mouse_life.gif"

// Static pointers
QMainWindow* mainWindow = nullptr;
QSystemTrayIcon* trayIcon = nullptr;
QLabel* dataLabel = nullptr;
QPushButton* reloadButton = nullptr;
QIcon* connectedIcon = nullptr;
QIcon* disconnectedIcon = nullptr;
QMediaPlayer* lowBatteryPlayer = nullptr;
QLabel* statusLabel = nullptr; // Replaced textbox with label

// New: labels for individual stats
QMap<QString, QLabel*> statLabels;

QString lastReadingTime = "Never";

QStringList statNames = {
    "Left clicks", "Right clicks", "Middle clicks",
    "Backward clicks", "Forward clicks",
    "Down scrolls", "Up scrolls"
};

Gui::Gui(QApplication& app, QObject* parent) : QObject(parent), app(app) {}
Gui::~Gui() {}

QString format_mouse_data(const ComPort::MouseStatus& data)
{
    return QString(
               "Left clicks: %1\nRight clicks: %2\nMiddle clicks: %3\n"
               "Backward clicks: %4\nForward clicks: %5\n"
               "Down scrolls: %6\nUp scrolls: %7\nBattery level: %8%%")
        .arg(data.left_clicks)
        .arg(data.right_clicks)
        .arg(data.middle_clicks)
        .arg(data.backward_clicks)
        .arg(data.forward_clicks)
        .arg(data.downward_scrolls)
        .arg(data.upward_scrolls)
        .arg(data.battery_percent);
}

void Gui::updateGui(ComPort::MouseStatus& data, bool connected)
{
    if (connected)
    {
        mainWindow->setWindowTitle("Connected to MOUSE");
        mainWindow->setWindowIcon(*connectedIcon);
        trayIcon->setIcon(*connectedIcon);
        trayIcon->setToolTip("Connected to MOUSE");
        reloadButton->setEnabled(true);
        reloadButton->setToolTip("Click to update the data");

        // Update stat labels with black color and larger text
        statLabels["Left clicks"]->setText(QString("<span style='color:black; font-weight:bold; font-size:14px;'>%1</span>").arg(data.left_clicks));
        statLabels["Right clicks"]->setText(QString("<span style='color:black; font-weight:bold; font-size:14px;'>%1</span>").arg(data.right_clicks));
        statLabels["Middle clicks"]->setText(QString("<span style='color:black; font-weight:bold; font-size:14px;'>%1</span>").arg(data.middle_clicks));
        statLabels["Backward clicks"]->setText(QString("<span style='color:black; font-weight:bold; font-size:14px;'>%1</span>").arg(data.backward_clicks));
        statLabels["Forward clicks"]->setText(QString("<span style='color:black; font-weight:bold; font-size:14px;'>%1</span>").arg(data.forward_clicks));
        statLabels["Down scrolls"]->setText(QString("<span style='color:black; font-weight:bold; font-size:14px;'>%1</span>").arg(data.downward_scrolls));
        statLabels["Up scrolls"]->setText(QString("<span style='color:black; font-weight:bold; font-size:14px;'>%1</span>").arg(data.upward_scrolls));
        statLabels["Battery level"]->setText(QString("<span style='color:black; font-weight:bold; font-size:14px;'>%1%</span>").arg(data.battery_percent));
        statLabels["Last reading"]->setText(QString("<span style='color:black; font-size:14px;'>%1</span>").arg(lastReadingTime));

        lastReadingTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        statLabels["Last reading"]->setText(QString("<span style='color:black; font-size:14px;'>%1</span>").arg(lastReadingTime));
        statusLabel->setText("<span style='color:green; font-size:12px;'>Connected at " + lastReadingTime + "</span>");

        // Append to CSV
        QFile file(STATS_FILENAME);
        bool needHeader = (!file.exists() || file.size() == 0);
        if (file.open(QIODevice::Append | QIODevice::Text))
        {
            QTextStream out(&file);
            if (needHeader)
            {
                out << "timestamp,left_clicks,right_clicks,middle_clicks,backward_clicks,forward_clicks,downward_scrolls,upward_scrolls\n";
            }
            out << lastReadingTime << "," << data.left_clicks << "," << data.right_clicks << ","
                << data.middle_clicks << "," << data.backward_clicks << "," << data.forward_clicks << ","
                << data.downward_scrolls << "," << data.upward_scrolls << "\n";
        }

        if (data.battery_percent < 30 && lowBatteryPlayer)
        {
            lowBatteryPlayer->play();
        }
    }
    else
    {
        mainWindow->setWindowTitle("Not connected");
        mainWindow->setWindowIcon(*disconnectedIcon);
        trayIcon->setIcon(*disconnectedIcon);
        trayIcon->setToolTip("Not connected");
        reloadButton->setEnabled(false);
        reloadButton->setToolTip("No device connected");

        // Keep last values, but gray
        for (const QString& name : statLabels.keys())
        {
            QString currentText = statLabels[name]->text();
            QString value = currentText.section('>', 1).section('<', 0, 0);
            statLabels[name]->setText(QString("<span style='color:gray; font-size:14px;'>%1</span>").arg(value));
        }
        statusLabel->setText("<span style='color:red; font-size:12px;'>Disconnected at " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "</span>");
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    this->hide();
    event->ignore();
}

void gui_init(QApplication& app, QAction** quitActionOut)
{
    // Load icons with QString() to ensure proper resource path
    connectedIcon = new QIcon(QString(":/res/icons/mouse.png"));
    disconnectedIcon = new QIcon(QString(":/res/icons/mouse_not.png"));
    if (connectedIcon->isNull() || disconnectedIcon->isNull()) {
        qDebug() << "Warning: One or both icons failed to load.";
    }
    app.setWindowIcon(*disconnectedIcon);

    QFile checkFile(STATS_FILENAME);
    if (!checkFile.exists())
    {
        if (checkFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&checkFile);
            out << "Date";
            for (const auto& name : statNames)
                out << "," << name;
            out << "\n";
        }
    }

    mainWindow = new MainWindow();
    mainWindow->setWindowTitle("Not connected");
    mainWindow->setWindowIcon(*disconnectedIcon);
    mainWindow->setFixedWidth(WINDOW_SIZE_X);         // Fixed width
    mainWindow->setFixedHeight(WINDOW_SIZE_Y);         // Fixed width

    // Menus (removed File menu)
    QMenu* aboutMenu = new QMenu("About");
    QAction* aboutAction = new QAction("About");
    QAction* thalesAction = new QAction("Thales");
    aboutMenu->addAction(aboutAction);
    aboutMenu->addAction(thalesAction);

    QMenuBar* menuBar = new QMenuBar();
    menuBar->addMenu(aboutMenu);
    mainWindow->setMenuBar(menuBar);

    // Main layout using QGridLayout
    QGridLayout* mainLayout = new QGridLayout();
    mainLayout->setSpacing(2); // Minimal spacing
    mainLayout->setContentsMargins(2, 2, 2, 2); // Minimal margins
    mainLayout->setAlignment(Qt::AlignTop);     // Align the entire layout to the top

    // Grid layout for stats
    QGridLayout* grid = new QGridLayout();
    grid->setSpacing(2); // Increased spacing to prevent overlap
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setAlignment(Qt::AlignTop);           // Align grid contents to the top

    QString nameStyle = "padding: 1px; font-size: 12px;"; // Increased from 10px to 12px
    QString valueStyle = "padding: 1px; font-weight: bold; font-size: 14px;"; // Increased from 12px to 14px

    for (int i = 0; i < statNames.size(); ++i)
    {
        QLabel* nameLabel = new QLabel(statNames[i] + ":");
        QLabel* valueLabel = new QLabel("<span style='color:gray; font-size:14px;'>0</span>");
        valueLabel->setTextFormat(Qt::RichText);

        nameLabel->setStyleSheet(nameStyle);
        valueLabel->setStyleSheet(valueStyle);

        statLabels[statNames[i]] = valueLabel;
        grid->addWidget(nameLabel, i, 0, Qt::AlignRight | Qt::AlignTop); // Align to top
        grid->addWidget(valueLabel, i, 1, Qt::AlignLeft | Qt::AlignTop); // Align to top
    }

    QLabel* batteryLabel = new QLabel("Battery level:");
    QLabel* batteryValueLabel = new QLabel("<span style='color:gray; font-size:14px;'>0%</span>");
    batteryValueLabel->setTextFormat(Qt::RichText);
    batteryValueLabel->setStyleSheet(valueStyle);
    batteryLabel->setStyleSheet(nameStyle);
    statLabels["Battery level"] = batteryValueLabel;
    grid->addWidget(batteryLabel, statNames.size(), 0, Qt::AlignRight | Qt::AlignTop);
    grid->addWidget(batteryValueLabel, statNames.size(), 1, Qt::AlignLeft | Qt::AlignTop);

    QLabel* lastReadingLabel = new QLabel("Last reading:");
    QLabel* lastReadingValueLabel = new QLabel("<span style='color:gray; font-size:14px;'>Never</span>");
    lastReadingValueLabel->setTextFormat(Qt::RichText);
    lastReadingValueLabel->setStyleSheet(valueStyle);
    lastReadingLabel->setStyleSheet(nameStyle);
    statLabels["Last reading"] = lastReadingValueLabel;
    grid->addWidget(lastReadingLabel, statNames.size() + 1, 0, Qt::AlignRight | Qt::AlignTop);
    grid->addWidget(lastReadingValueLabel, statNames.size() + 1, 1, Qt::AlignLeft | Qt::AlignTop);

    // Frame around grid
    QFrame* gridFrame = new QFrame();
    QVBoxLayout* frameLayout = new QVBoxLayout(gridFrame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->setAlignment(Qt::AlignTop);     // Align frame contents to the top
    frameLayout->addLayout(grid);
    mainLayout->addWidget(gridFrame, 0, 0, 1, 2, Qt::AlignTop); // Span two columns, align to top

    // Data label
    dataLabel = new QLabel();
    dataLabel->setTextFormat(Qt::RichText);
    dataLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    dataLabel->setMaximumHeight(40); // Limit height
    mainLayout->addWidget(dataLabel, 1, 0, 1, 2, Qt::AlignTop); // Span two columns, align to top

    // Reload button (full width)
    reloadButton = new QPushButton();
    reloadButton->setIcon(QIcon::fromTheme("view-refresh"));
    reloadButton->setToolTip("No device connected");
    reloadButton->setEnabled(false);
    mainLayout->addWidget(reloadButton, 2, 0, 1, 2, Qt::AlignTop); // Span two columns, align to top
    reloadButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // Expand horizontally

    // Status label (replaced textbox)
    statusLabel = new QLabel("<span style='color:gray; font-size:12px;'>Not connected</span>");
    statusLabel->setTextFormat(Qt::RichText);
    statusLabel->setAlignment(Qt::AlignCenter | Qt::AlignTop);
    statusLabel->setStyleSheet("font-size: 12px;"); // Increased from 10px to 12px
    mainLayout->addWidget(statusLabel, 3, 0, 1, 2, Qt::AlignTop); // Span two columns, align to top

    // Set layout
    QWidget* centralWidget = new QWidget();
    centralWidget->setLayout(mainLayout);
    mainWindow->setCentralWidget(centralWidget);

    trayIcon = new QSystemTrayIcon(mainWindow);
    trayIcon->setIcon(*disconnectedIcon); // Ensure icon is set before making visible
    trayIcon->setToolTip("Not connected");

    QMenu* trayMenu = new QMenu(mainWindow);
    QAction* openAction = new QAction("Open");
    QAction* quitAction = new QAction("Quit");
    trayMenu->addAction(openAction);
    trayMenu->addAction(quitAction);
    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();

    if (quitActionOut)
        *quitActionOut = quitAction;

    QObject::connect(aboutAction, &QAction::triggered, []()
                     {
        QMovie* movie = new QMovie(QString(":/res/") + GIF_FILENAME);
        if (movie->isValid())
        {
            QLabel* gifLabel = new QLabel();
            gifLabel->setMovie(movie);
            movie->start();

            QDialog dialog(mainWindow);
            QVBoxLayout* vbox = new QVBoxLayout(&dialog);
            vbox->addWidget(gifLabel);
            dialog.setWindowTitle("About");
            dialog.resize(400, 300); // Adjusted size for GIF
            dialog.exec();
        }
        else
        {
            QMessageBox::warning(mainWindow, "Error", QString("Failed to load GIF: ") + GIF_FILENAME);
        }
                     });

    QObject::connect(thalesAction, &QAction::triggered, []()
                     {
        QFile file(QString(":/res/") + THALES_FILENAME);
        QString text = "Could not load text.";
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
            text = QTextStream(&file).readAll();

        QTextBrowser* browser = new QTextBrowser;
        browser->setText(text);
        browser->setFrameStyle(QFrame::NoFrame);
        browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        browser->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        browser->setTextInteractionFlags(Qt::NoTextInteraction);
        browser->setStyleSheet("background: palette(window); color: palette(text);");

        QDialog dialog(mainWindow);
        QVBoxLayout* vbox = new QVBoxLayout(&dialog);
        vbox->addWidget(browser);
        dialog.setWindowTitle("Mouse Thales");
        dialog.resize(400, 300);
        dialog.exec(); });

    QObject::connect(openAction, &QAction::triggered, []()
                     {
        mainWindow->show();
        mainWindow->raise();
        mainWindow->activateWindow(); });

    // Load last record if present
    QFile stats(STATS_FILENAME);
    if (stats.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QString lastLine;
        QTextStream in(&stats);
        while (!in.atEnd())
            lastLine = in.readLine();
        auto fields = lastLine.split(",");
        if (fields.size() >= statNames.size() + 1)
        {
            lastReadingTime = fields[0];
            statLabels["Last reading"]->setText(QString("<span style='color:gray; font-size:14px;'>%1</span>").arg(lastReadingTime));
            for (int i = 0; i < statNames.size(); ++i)
            {
                statLabels[statNames[i]]->setText(
                    QString("<span style='color:gray; font-size:14px;'>%1</span>").arg(fields[i + 1]));
            }
        }
    }

    lowBatteryPlayer = new QMediaPlayer(mainWindow);
    QAudioOutput* audioOutput = new QAudioOutput(mainWindow);
    audioOutput->setVolume(1.0);
    lowBatteryPlayer->setAudioOutput(audioOutput);
    lowBatteryPlayer->setSource(QUrl(QString(":/res/") + BEEP_FILENAME));

    new Gui(app, mainWindow);
}