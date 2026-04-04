#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QLabel>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>

#include "bkk_touchscreen.hpp"
#include "bkk_touchscreen_feedback.hpp"
#include "bkk_arrival_table_handler.hpp"
#include "bkk_info_bar_handler.hpp"


class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupUi();

    BkkTouchScreenWorker *touchscreenWorker = nullptr;
    void setupTouchScreenWorker();
    static void touchscreenCallback(ts_event_en event, void * arg); 
    ts_event_en currentTouchEvent = TOUCHSCREEN_EVENT_RELEASED;
    TouchScreenFeedBack *touchFeedback = nullptr;

    BkkInfoBarHandler *infoBarHandler = nullptr;
    ArrivalTableHandler *arrivalTableHandler = nullptr;

    QTimer touchScreenWorkerTimer; 
};

#endif