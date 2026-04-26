#include "mainwindow.hpp"
#include "bkk_touchscreen.hpp"

#include <QApplication>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QPixmap>
#include <QVBoxLayout>
#include <rbuflogd/producer.h>

#include "bkk_screenshot_util.hpp"
#include <csignal>
#include <signal.h>
#include <unistd.h>


static void sigHandler(int signum) {
    if(signum == SIGUSR1) {
       QScreen *screen = QGuiApplication::primaryScreen();
        if(screen != nullptr) {
            QMetaObject::invokeMethod(QApplication::instance(), [screen]() {
                ScreenshotUtil util("/tmp/bkk_screenshot.png");
                util.saveScreenshot(screen);
            }, Qt::QueuedConnection);  
        }
        else {
            // temporary removed logging
        }
    }
}


MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    rbuflogd_producer_open(&loggerProducer, "MainWindow");

    setupUi();

    // Initialize touchscreen callback
    setupTouchScreenWorker(); 
    
    // Setup touch feedback overlay (raised above child widgets)
    touchFeedback = new TouchScreenFeedBack(new QLabel(this));

    // Register signal handler for SIGUSR1 to trigger screenshot
    if(signal(SIGUSR1, sigHandler) == SIG_ERR) {
        rbuflogd_producer_log(
            &loggerProducer, 
            RBUF_LOG_LEVEL_ERROR, 
            "Init", 
            "Failed to register signal handler for SIGUSR1");
    }
    else {
        rbuflogd_producer_log(
            &loggerProducer, 
            RBUF_LOG_LEVEL_INFO, 
            "Init", 
            "Signal handler for SIGUSR1 registered successfully");
    }

}

MainWindow::~MainWindow()
{
    rbuflogd_producer_close(&loggerProducer);
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
    infoBarHandler = new BkkInfoBarHandler(statusRow);
    layout->addWidget(statusRow);

    auto *arrivalsTable = new QTableWidget(this);
    arrivalTableHandler = new ArrivalTableHandler(arrivalsTable);
    layout->addWidget(arrivalsTable, 1);
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

