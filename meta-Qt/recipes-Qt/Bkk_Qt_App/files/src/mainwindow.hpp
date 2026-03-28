#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QColor>
#include <QLabel>
#include <QString>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>

#include "bkk_worker_thread.hpp"
#include "bkk_touchscreen.hpp"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupUi();
    void setupTableWidget();
    void startWorkerThread();
    void stopWorkerThread();
    void startTimers();
    void stopTimers();
    void handleApiFetchCompleted();
    void handleClockUpdateCompleted();
    void handleOnlineCheckCompleted();
    void updateUi();
    void populateArrivalsTable();
    void showTableMessage(const QString &message);
    QColor getRowColor(int row) const;
    QWidget *createDepartureCell(int departsInMin, const QColor &backgroundColor) const;
    
    BkkTouchScreenWorker *touchscreenWorker = nullptr;
    void setupTouchScreenWorker();
    static void touchscreenCallback(ts_event_en event, void * arg); 
    ts_event_en currentTouchEvent = TOUCHSCREEN_EVENT_RELEASED;

    QLabel *clockLabel;
    QLabel *bkkLogoLabel;
    QLabel *wifiIconLabel;
    QTableWidget *arrivalsTable;


    WorkerThread workerThread;

    std::vector<StationArrival> arrivals;
    BkkApiError apiError;
    std::string clockText;
    bool onlineStatus;

    QTimer clockUpdateTimer;
    QTimer bkkApiFetchTimer;
    QTimer onlineCheckTimer;
    QTimer mainTaskTimer;
    QTimer touchScreenWorkerTimer; 
    bool blinkOn;
};

#endif