#include "mainwindow.hpp"
#include "bkk_touchscreen.hpp"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QPixmap>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "bkk_logger.hpp"

#include <algorithm>
#include <cstdio>


MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent),
      clockLabel(nullptr),
      bkkLogoLabel(nullptr),
      wifiIconLabel(nullptr),
      clockText(),
      onlineStatus(false),
      blinkOn(false)
{

    setupUi();

    

    startWorkerThread();
    startTimers();

    // Initialize touchscreen callback
    setupTouchScreenWorker(); 
    
    // Setup touch feedback overlay (raised above child widgets)
    touchFeedback = new TouchScreenFeedBack(new QLabel(this));

}

MainWindow::~MainWindow()
{
    stopTimers();
    stopWorkerThread();
}

void MainWindow::setupUi()
{
    setMinimumSize(480, 320);
    setWindowTitle("BKK Display");

    setStyleSheet(
        "QWidget { background-color: #340a41; color: #ffffff; }"
        "QLabel  { background-color: #340a41; color: #ffffff; }"
        "QHeaderView::section { background-color: #505050; color: #ffffff; "
        "                        border: none; padding: 4px; font-weight: bold; }"
        "QTableWidget { background-color: #340a41; gridline-color: #505050;"
        "                border: none; }"
        "QTableCornerButton::section { background-color: #505050; }"
    );

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto *statusRow = new QWidget(this);
    statusRow->setFixedHeight(46);
    auto *statusRowLayout = new QHBoxLayout(statusRow);
    statusRowLayout->setContentsMargins(0, 0, 0, 0);
    statusRowLayout->setSpacing(8);

    clockLabel = new QLabel(statusRow);
    clockLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    clockLabel->setFixedWidth(86);

    bkkLogoLabel = new QLabel(statusRow);
    bkkLogoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    bkkLogoLabel->setFixedSize(106, 40);
    statusRowLayout->addWidget(bkkLogoLabel);

    statusRowLayout->addStretch(1);

    wifiIconLabel = new QLabel(statusRow);
    wifiIconLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    wifiIconLabel->setFixedSize(40, 40);
    statusRowLayout->addWidget(wifiIconLabel);

    statusRowLayout->addSpacing(12);

    statusRowLayout->addWidget(clockLabel);

    layout->addWidget(statusRow);

    arrivalsTable = new QTableWidget(this);
    arrivalTableHandler = new ArrivalTableHandler(arrivalsTable);
    layout->addWidget(arrivalsTable, 1);
}


void MainWindow::startWorkerThread()
{
    QObject::connect(&workerThread, &WorkerThread::clockUpdateCompleted, this, &MainWindow::handleClockUpdateCompleted);
    QObject::connect(&workerThread, &WorkerThread::onlineCheckCompleted, this, &MainWindow::handleOnlineCheckCompleted);

    if (!workerThread.isRunning()) {
        workerThread.start();
    }
}

void MainWindow::stopWorkerThread()
{
    if (!workerThread.isRunning()) {
        return;
    }

    workerThread.requestInterruption();
    workerThread.wait();
}

void MainWindow::startTimers()
{
    workerThread.requestClockUpdate();
    workerThread.requestOnlineCheck();

    QObject::connect(&clockUpdateTimer, &QTimer::timeout, this, [this]() {
        workerThread.requestClockUpdate();
    });
    clockUpdateTimer.start(1000);

    QObject::connect(&onlineCheckTimer, &QTimer::timeout, this, [this]() {
        workerThread.requestOnlineCheck();
    });
    onlineCheckTimer.start(5000);

    QObject::connect(&mainTaskTimer, &QTimer::timeout, this, [this]() {
        updateUi();

        // flip the blink state for the departure dots
        blinkOn = !blinkOn;
    });
    mainTaskTimer.start(1000);

    updateUi();
}

void MainWindow::handleClockUpdateCompleted()
{
    clockText = workerThread.getClockText();
    updateUi();
}

void MainWindow::handleOnlineCheckCompleted()
{
    onlineStatus = workerThread.isOnline();
    updateUi();
}

void MainWindow::stopTimers()
{
    mainTaskTimer.stop();
    onlineCheckTimer.stop();
    clockUpdateTimer.stop();
}

void MainWindow::updateUi()
{
    clockLabel->setText(QString::fromStdString(clockText));

    wifiIconLabel->setPixmap(
        QPixmap(onlineStatus ? ":/icons/wifi_on.png" : ":/icons/wifi_off.png")
            .scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    bkkLogoLabel->setPixmap(
        QPixmap(":/icons/bkk_logo.png")
            .scaled(106, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));

}


void MainWindow::setupTouchScreenWorker() {
    touchscreenWorker = new BkkTouchScreenWorker(
        touchscreenCallback, this);

    // setup worker timer but do not start it yet:
    QObject::connect(
        &touchScreenWorkerTimer, 
        &QTimer::timeout, this, [this]() {
            if (touchscreenWorker != nullptr) {
                if (touchscreenWorker->fetch_touch_coordinates() == 0) {
                    touchFeedback->showTouchAt(
                        touchscreenWorker->getX(), 
                        touchscreenWorker->getY());                  
                }
            }
        }
    );
}


void MainWindow::touchscreenCallback(ts_event_en event, void * arg) {
    (void)event; // Unused parameter
    
    MainWindow *self = static_cast<MainWindow *>(arg);
    if (self == nullptr) {
        return;
    }

    if(event == TOUCHSCREEN_EVENT_TOUCHED
         && self->currentTouchEvent != TOUCHSCREEN_EVENT_TOUCHED) {
        
        self->currentTouchEvent = TOUCHSCREEN_EVENT_TOUCHED;
        // Start timer to fetch coordinates 
        QMetaObject::invokeMethod(self, [self]() {
            self->touchScreenWorkerTimer.start(30); 
        }, Qt::QueuedConnection);
    }
    else if(event == TOUCHSCREEN_EVENT_RELEASED
         && self->currentTouchEvent != TOUCHSCREEN_EVENT_RELEASED) {
        self->currentTouchEvent = TOUCHSCREEN_EVENT_RELEASED;
        // Stop fetching coordinates when touch is released
        QMetaObject::invokeMethod(self, [self]() {
            self->touchScreenWorkerTimer.stop();
        }, Qt::QueuedConnection);
    }  
    else {
        // Ignore other events or repeated events
    }
}

