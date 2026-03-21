#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QColor>
#include <QLabel>
#include <QString>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>

#include "bkk_api_wrapper.hpp"
#include "bkk_clock_update.hpp"
#include "bkk_online_check.hpp"

class MainWindow : public QWidget
{
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupUi();
    void setupTableWidget();
    void startTimers();
    void stopTimers();
    void updateUi();
    void populateArrivalsTable();
    void showTableMessage(const QString &message);
    QColor getRowColor(int row) const;
    QWidget *createDepartureCell(int departsInMin, const QColor &backgroundColor) const;

    QLabel *clockLabel;
    QLabel *bkkLogoLabel;
    QLabel *wifiIconLabel;
    QTableWidget *arrivalsTable;

    CLockUpdater clockUpdater;
    BkkApiWrapper apiWrapper;
    OnlineChecker onlineChecker;

    QTimer clockUpdateTimer;
    QTimer bkkApiFetchTimer;
    QTimer onlineCheckTimer;
    QTimer mainTaskTimer;
    bool blinkOn;
};

#endif