#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QLabel>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>


class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupUi();
};

#endif