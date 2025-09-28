#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QDebug>
#include <QDialog>
#include <QFile>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMovie>
#include <QPushButton>
#include <QScrollArea>
#include <QSystemTrayIcon>
#include <QTextBrowser>
#include <QTextStream>
#include <QTimeZone>
#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QMediaPlayer>
#include <QVBoxLayout>

#include "../include/gui.hpp"

#define APP_VERSION "0.9"

#define WINDOW_SIZE_X 300
#define WINDOW_SIZE_Y 280

#define STATS_FILENAME "mouse_stats.txt"
#define THALES_FILENAME "mouse_thales.txt"
#define BEEP_FILENAME "beep.wav"
#define GIF_FILENAME "mouse_life_downsized.gif"

// Static pointers
QMainWindow *mainWindow = nullptr;
QSystemTrayIcon *trayIcon = nullptr;
QLabel *dataLabel = nullptr;
QPushButton *connectButton = nullptr;
QIcon *connectedIcon = nullptr;
QIcon *disconnectedIcon = nullptr;
QMediaPlayer *lowBatteryPlayer = nullptr;
QAudioOutput *lowBatteryAudio = nullptr;

// New: labels for individual stats
QMap<QString, QLabel *> statLabels;

QString Gui::lastReadingTime = "Never";

QStringList statNames = {
    "Left clicks", "Right clicks", "Middle clicks",
    "Backward clicks", "Forward clicks",
    "Down scrolls", "Up scrolls",
    "Current DPI"};

Gui::Gui(QApplication &app, QObject *parent) : QObject(parent), app(app) {}
Gui::~Gui() {}

bool Gui::guiOpen = false;

void Gui::updateGui(ComPort::MouseStatus &data, bool connected)
{
    if (connected)
    {
        if (ComPort::connectedTo == ComPort::Subject::MOUSE)
        {
            mainWindow->setWindowTitle("Connected to MOUSE");
            trayIcon->setToolTip("Connected to MOUSE");
        }
        else if (ComPort::connectedTo == ComPort::Subject::RECEIVER)
        {
            for (const QString &name : statLabels.keys())
            {
                QString currentText = statLabels[name]->text();
                QString value = currentText.section('>', 1).section('<', 0, 0);
                statLabels[name]->setText(QString("<span style='color:gray; font-size:14px;'>%1</span>").arg(value));
            }

            statLabels["Current DPI"]->setText(QString("<span style='color:gray; font-size:14px;'>-</span>"));

            mainWindow->setWindowTitle("Connected to RECEIVER");
            trayIcon->setToolTip("Connected to RECEIVER");
        }

        mainWindow->setWindowIcon(*connectedIcon);
        trayIcon->setIcon(*connectedIcon);

        // Update stat labels with black color and larger text
        if (ComPort::connectedTo == ComPort::Subject::MOUSE)
        {
            statLabels["Left clicks"]->setText(QString("<span style='color:black; font-weight:bold; font-size:14px;'>%1</span>").arg(data.left_clicks));
            statLabels["Right clicks"]->setText(QString("<span style='color:black; font-weight:bold; font-size:14px;'>%1</span>").arg(data.right_clicks));
            statLabels["Middle clicks"]->setText(QString("<span style='color:black; font-weight:bold; font-size:14px;'>%1</span>").arg(data.middle_clicks));
            statLabels["Backward clicks"]->setText(QString("<span style='color:black; font-weight:bold; font-size:14px;'>%1</span>").arg(data.backward_clicks));
            statLabels["Forward clicks"]->setText(QString("<span style='color:black; font-weight:bold; font-size:14px;'>%1</span>").arg(data.forward_clicks));
            statLabels["Down scrolls"]->setText(QString("<span style='color:black; font-weight:bold; font-size:14px;'>%1</span>").arg(data.downward_scrolls));
            statLabels["Up scrolls"]->setText(QString("<span style='color:black; font-weight:bold; font-size:14px;'>%1</span>").arg(data.upward_scrolls));
            statLabels["Current DPI"]->setText(QString("<span style='color:black; font-weight:bold; font-size:14px;'>%1</span>").arg(data.current_dpi));
        }

        statLabels["Battery level"]->setText(QString("<span style='color:black; font-weight:bold; font-size:14px;'>%1%</span>").arg(data.battery_percent));
        statLabels["Last reading"]->setText(QString("<span style='color:black; font-size:14px;'>%1</span>").arg(Gui::lastReadingTime));

        // Append to CSV (do NOT include DPI)
        if (ComPort::connectedTo == ComPort::Subject::MOUSE)
        {
            QFile file(STATS_FILENAME);
            bool needHeader = (!file.exists() || file.size() == 0);
            if (file.open(QIODevice::Append | QIODevice::Text))
            {
                QTextStream out(&file);
                if (needHeader)
                {
                    out << "timestamp,left_clicks,right_clicks,middle_clicks,backward_clicks,forward_clicks,downward_scrolls,upward_scrolls\n";
                }
                out << Gui::lastReadingTime << "," << data.left_clicks << "," << data.right_clicks << ","
                    << data.middle_clicks << "," << data.backward_clicks << "," << data.forward_clicks << ","
                    << data.downward_scrolls << "," << data.upward_scrolls << "\n";

                std::cout << "Wrote stats to file." << std::endl;
            }
        }

        if (data.battery_percent < 30 && lowBatteryPlayer)
        {
            statLabels["Battery level"]->setText(QString("<span style='color:red; font-weight:bold; font-size:14px;'>%1%</span>").arg(data.battery_percent));
            lowBatteryPlayer->play();
        }
    }
    else
    {
        mainWindow->setWindowTitle("Not connected");
        mainWindow->setWindowIcon(*disconnectedIcon);
        trayIcon->setIcon(*disconnectedIcon);
        trayIcon->setToolTip("Not connected");

        // Keep last values, but gray
        for (const QString &name : statLabels.keys())
        {
            QString currentText = statLabels[name]->text();
            QString value = currentText.section('>', 1).section('<', 0, 0);
            statLabels[name]->setText(QString("<span style='color:gray; font-size:14px;'>%1</span>").arg(value));
        }
    }

    std::cout << "GUI updated" << std::endl;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    this->hide();
    event->ignore();
    Gui::guiOpen = false;
}

void gui_init(QApplication &app, QAction **quitActionOut)
{
    Gui::guiOpen = false;

    // Load icons with QString() to ensure proper resource path
    connectedIcon = new QIcon(QString(":/res/icons/mouse.png"));
    disconnectedIcon = new QIcon(QString(":/res/icons/mouse_not.png"));
    if (connectedIcon->isNull() || disconnectedIcon->isNull())
    {
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
            for (const auto &name : statNames)
                if (name != "Current DPI")
                    out << "," << name;
            out << "\n";
        }
    }

    mainWindow = new MainWindow();
    mainWindow->setWindowTitle("Not connected");
    mainWindow->setWindowIcon(*disconnectedIcon);
    mainWindow->setFixedWidth(WINDOW_SIZE_X);
    mainWindow->setFixedHeight(WINDOW_SIZE_Y);

    QMenu *aboutMenu = new QMenu("About");
    QAction *aboutAction = new QAction("About");
    QAction *thalesAction = new QAction("Thales");
    aboutMenu->addAction(aboutAction);
    aboutMenu->addAction(thalesAction);

    QMenuBar *menuBar = new QMenuBar();
    menuBar->addMenu(aboutMenu);
    mainWindow->setMenuBar(menuBar);

    QGridLayout *mainLayout = new QGridLayout();
    mainLayout->setSpacing(2);
    mainLayout->setContentsMargins(2, 2, 2, 2);
    mainLayout->setAlignment(Qt::AlignTop);

    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(2);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setAlignment(Qt::AlignTop);
    grid->setColumnStretch(0, 0);
    grid->setColumnStretch(1, 1);

    QString nameStyle = "padding: 1px; font-size: 12px;";
    QString valueStyle = "padding: 1px; font-weight: bold; font-size: 14px;";

    for (int i = 0; i < statNames.size(); ++i)
    {
        QLabel *nameLabel = new QLabel(statNames[i] + ":");
        QLabel *valueLabel = new QLabel("<span style='color:gray; font-size:14px;'>-</span>");
        valueLabel->setTextFormat(Qt::RichText);

        nameLabel->setStyleSheet(nameStyle);
        valueLabel->setStyleSheet(valueStyle);

        statLabels[statNames[i]] = valueLabel;
        grid->addWidget(nameLabel, i, 0, Qt::AlignLeft | Qt::AlignTop);
        grid->addWidget(valueLabel, i, 1, Qt::AlignLeft | Qt::AlignTop);
    }

    QLabel *batteryLabel = new QLabel("Battery level:");
    QLabel *batteryValueLabel = new QLabel("<span style='color:gray; font-size:14px;'>-</span>");
    batteryValueLabel->setTextFormat(Qt::RichText);
    batteryValueLabel->setStyleSheet(valueStyle);
    batteryLabel->setStyleSheet(nameStyle);
    statLabels["Battery level"] = batteryValueLabel;
    grid->addWidget(batteryLabel, statNames.size(), 0, Qt::AlignLeft | Qt::AlignTop);
    grid->addWidget(batteryValueLabel, statNames.size(), 1, Qt::AlignLeft | Qt::AlignTop);

    QLabel *lastReadingLabel = new QLabel("Last reading:");
    QLabel *lastReadingValueLabel = new QLabel("<span style='color:gray; font-size:14px;'>-</span>");
    lastReadingValueLabel->setTextFormat(Qt::RichText);
    lastReadingValueLabel->setStyleSheet(valueStyle);
    lastReadingLabel->setStyleSheet(nameStyle);
    statLabels["Last reading"] = lastReadingValueLabel;
    grid->addWidget(lastReadingLabel, statNames.size() + 1, 0, Qt::AlignLeft | Qt::AlignTop);
    grid->addWidget(lastReadingValueLabel, statNames.size() + 1, 1, Qt::AlignLeft | Qt::AlignTop);

    QFrame *gridFrame = new QFrame();
    QVBoxLayout *frameLayout = new QVBoxLayout(gridFrame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->setAlignment(Qt::AlignTop);
    frameLayout->addLayout(grid);
    mainLayout->addWidget(gridFrame, 0, 0, 1, 2, Qt::AlignTop);

    dataLabel = new QLabel();
    dataLabel->setTextFormat(Qt::RichText);
    dataLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    dataLabel->setMaximumHeight(40);
    mainLayout->addWidget(dataLabel, 1, 0, 1, 2, Qt::AlignTop);

    QWidget *centralWidget = new QWidget();
    centralWidget->setLayout(mainLayout);
    mainWindow->setCentralWidget(centralWidget);

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

    if (quitActionOut)
        *quitActionOut = quitAction;

    QObject::connect(aboutAction, &QAction::triggered, []()
                     {
    QMovie *movie = new QMovie(QString(":/res/") + GIF_FILENAME);
    if (!movie->isValid()) {
        QMessageBox::warning(mainWindow, "Error", QString("Failed to load GIF: ") + GIF_FILENAME);
        return;
    }
    QLabel *gifLabel = new QLabel();
    movie->jumpToFrame(0);
    QSize gifSize = movie->currentImage().size();
    gifLabel->setFixedSize(gifSize);
    gifLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    gifLabel->setMovie(movie);
    movie->start();

    QString buildInfo = QString("<b>Mouse Client</b><br>Display and backup mouse stats<br><br>Version: %1<br>Compiled on: %2 %3 %4")
                            .arg(APP_VERSION)
                            .arg(__DATE__)
                            .arg(__TIME__)
                            .arg(QTimeZone::systemTimeZone().displayName(QDateTime::currentDateTime(), QTimeZone::ShortName));

    QLabel *infoLabel = new QLabel(buildInfo);
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setTextFormat(Qt::RichText);
    infoLabel->setStyleSheet("font-size: 12px; padding: 4px;");

    QDialog dialog(mainWindow);
    dialog.setWindowTitle("About");
    dialog.setFixedSize(400, 150);

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->addWidget(gifLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);
    hbox->addSpacing(10);
    hbox->addWidget(infoLabel, 1, Qt::AlignCenter);
    dialog.setLayout(hbox);
    dialog.exec(); });

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
        mainWindow->activateWindow();
        Gui::guiOpen = true; });

    QFile stats(STATS_FILENAME);
    if (stats.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QString lastLine;
        QTextStream in(&stats);
        while (!in.atEnd())
            lastLine = in.readLine();
        auto fields = lastLine.split(",");
        if (fields.size() >= statNames.size()) // DPI is skipped in file, so only older fields loaded
        {
            Gui::lastReadingTime = fields[0];
            statLabels["Last reading"]->setText(QString("<span style='color:gray; font-size:14px;'>%1</span>").arg(Gui::lastReadingTime));
            for (int i = 0; i < statNames.size(); ++i)
            {
                if (statNames[i] == "Current DPI")
                    continue; // not in CSV
                statLabels[statNames[i]]->setText(
                    QString("<span style='color:gray; font-size:14px;'>%1</span>").arg(fields[i + 1]));
            }
        }
    }

    // MP3 low battery alert
    lowBatteryPlayer = new QMediaPlayer(mainWindow);
    lowBatteryAudio = new QAudioOutput(mainWindow);
    lowBatteryAudio->setVolume(1.0);
    lowBatteryPlayer->setAudioOutput(lowBatteryAudio);
    lowBatteryPlayer->setSource(QUrl(QStringLiteral("qrc:/res/") + BEEP_FILENAME));

    new Gui(app, mainWindow);
}
