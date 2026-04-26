#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QLabel>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>
#include <rbuflogd/producer.h>


class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupUi();

    rbuflogd_producer_t loggerProducer {};
};

#endif