// test/main.cpp
#include <QApplication>
#include <QTimer>
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QWidget>
#include <QSplitter>
#include "MainWindow.h"
#include "selfamplifer.h"

class TestWidget : public QWidget
{
    Q_OBJECT

public:
    TestWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
        setupUI();
        setupAmplifier();
        setupTimer();
    }

private slots:
    void onStartClicked()
    {
        if (amplifier) {
            amplifier->start();
            logText->append("Start data collection");
            updateStatus();
        }
    }

    void onStopClicked()
    {
        if (amplifier) {
            amplifier->stop();
            logText->append("Stop data collection");
            updateStatus();
        }
    }

    void onShowConnectWidget()
    {
        if (amplifier) {
            QWidget* connectWidget = amplifier->getConnectWidget();
            if (connectWidget) {
                connectWidget->show();
                connectWidget->raise();
                logText->append("Show bluetooth connection window");
            }
        }
    }

    void onTestChannelInfo()
    {
        if (amplifier) {
            QStringList channels = amplifier->getChannelName();
            quint8 channelNum = amplifier->getChannnelNum();
            quint16 sampleRate = amplifier->getSampleRate();

            QString info = QString("Channel Info:\n"
                "Channel Count: %1\n"
                "Channel Names: %2\n"
                "Sample Rate: %3 Hz\n"
                "EEG Index: %4")
                .arg(channelNum)
                .arg(channels.join(", "))
                .arg(sampleRate)
                .arg(listToString(amplifier->getEEGIndex()));

            logText->append(info);
        }
    }

    void onTestDataRetrieval()
    {
        if (amplifier) {
            // Test chart data retrieval
            QList<QList<double>> chartData = amplifier->getChartData();
            if (!chartData.isEmpty()) {
                QString info = QString("Chart data received: %1 samples")
                    .arg(chartData.size());
                logText->append(info);

                // Show first few samples
                for (int i = 0; i < qMin(3, chartData.size()); ++i) {
                    if (chartData[i].size() >= 2) {
                        QString sampleInfo = QString("Sample %1: FP1=%2, FP2=%3")
                            .arg(i + 1)
                            .arg(chartData[i][0], 0, 'f', 2)
                            .arg(chartData[i][1], 0, 'f', 2);
                        logText->append(sampleInfo);
                    }
                }
            }
            else {
                logText->append("No chart data available");
            }

            // Test raw data retrieval
            QList<QList<double>> rawData = amplifier->getRawData();
            if (!rawData.isEmpty() && !rawData[0].isEmpty()) {
                QString info = QString("Raw data received: %1 points")
                    .arg(rawData[0].size());
                logText->append(info);
            }
            else {
                logText->append("No raw data available");
            }
        }
    }

    void updateStatus()
    {
        if (amplifier) {
            bool connected = amplifier->connectStatus();
            QString status = connected ? "Connected" : "Disconnected";
            statusLabel->setText("Connection Status: " + status);

            // Update button states
            startBtn->setEnabled(connected);
            stopBtn->setEnabled(connected);
        }
    }

    void onPeriodicUpdate()
    {
        updateStatus();

        // Show data info periodically during collection
        static int updateCount = 0;
        updateCount++;

        if (updateCount % 10 == 0) { // Show data info every 10 updates
            onTestDataRetrieval();
        }
    }

private:
    void setupUI()
    {
        setWindowTitle("SelfAmplifer Function Test");
        setMinimumSize(800, 600);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // Status label
        statusLabel = new QLabel("Connection Status: Unknown");
        statusLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 5px; border: 1px solid #ccc; }");
        mainLayout->addWidget(statusLabel);

        // Button area
        QHBoxLayout* buttonLayout = new QHBoxLayout();

        QPushButton* showConnectBtn = new QPushButton("Show Connect Window");
        QPushButton* testInfoBtn = new QPushButton("Test Channel Info");
        startBtn = new QPushButton("Start Collection");
        stopBtn = new QPushButton("Stop Collection");
        QPushButton* testDataBtn = new QPushButton("Test Data Retrieval");

        connect(showConnectBtn, &QPushButton::clicked, this, &TestWidget::onShowConnectWidget);
        connect(testInfoBtn, &QPushButton::clicked, this, &TestWidget::onTestChannelInfo);
        connect(startBtn, &QPushButton::clicked, this, &TestWidget::onStartClicked);
        connect(stopBtn, &QPushButton::clicked, this, &TestWidget::onStopClicked);
        connect(testDataBtn, &QPushButton::clicked, this, &TestWidget::onTestDataRetrieval);

        buttonLayout->addWidget(showConnectBtn);
        buttonLayout->addWidget(testInfoBtn);
        buttonLayout->addWidget(startBtn);
        buttonLayout->addWidget(stopBtn);
        buttonLayout->addWidget(testDataBtn);
        buttonLayout->addStretch();

        mainLayout->addLayout(buttonLayout);

        // Log display area
        QLabel* logLabel = new QLabel("Test Log:");
        mainLayout->addWidget(logLabel);

        logText = new QTextEdit();
        logText->setMaximumHeight(400);
        mainLayout->addWidget(logText);
    }

    void setupAmplifier()
    {
        amplifier = new SelfAmplifer();  // Fixed: removed parent parameter

        // Connect amplifier signals if available
        // Note: Adjust these connections based on your actual SelfAmplifer implementation

        logText->append("SelfAmplifer plugin initialized");

        // Show basic info
        onTestChannelInfo();
    }

    void setupTimer()
    {
        // Setup timer for periodic status updates
        statusTimer = new QTimer(this);
        connect(statusTimer, &QTimer::timeout, this, &TestWidget::onPeriodicUpdate);
        statusTimer->start(1000); // Update every second
    }

    QString listToString(const QList<uint8_t>& list)
    {
        QStringList strList;
        for (uint8_t val : list) {
            strList << QString::number(val);
        }
        return "[" + strList.join(", ") + "]";
    }

private:
    SelfAmplifer* amplifier;
    QLabel* statusLabel;
    QPushButton* startBtn;
    QPushButton* stopBtn;
    QTextEdit* logText;
    QTimer* statusTimer;
};

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    // Register necessary meta types
    qRegisterMetaType<QList<double>>("QList<double>");
    qRegisterMetaType<DataParser::ParsedData>("DataParser::ParsedData");

    qDebug() << "Starting SelfAmplifer function test program";

    // Create test window
    TestWidget testWidget;
    testWidget.show();

    // Output test instructions
    qDebug() << "=== SelfAmplifer Test Instructions ===";
    qDebug() << "1. Click 'Show Connect Window' to open bluetooth connection interface";
    qDebug() << "2. Scan and connect device in bluetooth interface";
    qDebug() << "3. After successful connection, click 'Start Collection' to begin data collection";
    qDebug() << "4. Click 'Test Data Retrieval' to view received data";
    qDebug() << "5. Click 'Stop Collection' to stop data collection";
    qDebug() << "======================================";

    return a.exec();
}

#include "main.moc"