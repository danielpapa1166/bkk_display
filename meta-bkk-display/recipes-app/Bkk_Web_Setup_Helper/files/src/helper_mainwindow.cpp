#include "helper_mainwindow.hpp"

#include <QHBoxLayout>
#include <QPixmap>
#include <QVBoxLayout>



MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    rbuflogd_producer_open(&loggerProducer, "BkkConfig"); 
    setupUi();
}

MainWindow::~MainWindow()
{
    rbuflogd_producer_close(&loggerProducer);
}

void MainWindow::setupUi()
{
    rbuflogd_producer_log(&loggerProducer, RBUF_LOG_LEVEL_INFO, "Init", "Setting up UI");
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

    auto *iconLabel = new QLabel(this);
    QPixmap pixmap(":/icon/config.png");
    iconLabel->setPixmap(pixmap);
    iconLabel->setAlignment(Qt::AlignCenter);

    auto *textLabel = new QLabel("Connect to BKK Display Wifi \nand open 192.168.4.1:8080", this);
    textLabel->setAlignment(Qt::AlignCenter);

    layout->addStretch();
    layout->addWidget(iconLabel);
    layout->addWidget(textLabel);
    layout->addStretch();

    rbuflogd_producer_log(&loggerProducer, RBUF_LOG_LEVEL_INFO, "Init", "Setup UI completed");
}
