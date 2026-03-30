#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QColor>
#include <QLabel>
#include <QString>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>

#include "bkk_touchscreen.hpp"
#include "bkk_touchscreen_feedback.hpp"
#include "bkk_arrival_table_handler.hpp"
#include "bkk_worker_thread.hpp"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupUi();
    void startWorkerThread();
    void stopWorkerThread();
    void startTimers();
    void stopTimers();
    void handleClockUpdateCompleted();
    void handleOnlineCheckCompleted();
    void updateUi();

    BkkTouchScreenWorker *touchscreenWorker = nullptr;
    void setupTouchScreenWorker();
    static void touchscreenCallback(ts_event_en event, void * arg); 
    ts_event_en currentTouchEvent = TOUCHSCREEN_EVENT_RELEASED;
    TouchScreenFeedBack *touchFeedback = nullptr;


    QLabel *clockLabel;
    QLabel *bkkLogoLabel;
    QLabel *wifiIconLabel;
    QTableWidget *arrivalsTable;

    ArrivalTableHandler *arrivalTableHandler = nullptr;
    WorkerThread workerThread;

    std::string clockText;
    bool onlineStatus;

    QTimer clockUpdateTimer;
    QTimer onlineCheckTimer;
    QTimer mainTaskTimer;
    QTimer touchScreenWorkerTimer; 
    bool blinkOn;
};

#endif