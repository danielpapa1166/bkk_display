#include <QApplication>
#include <rbuflogd/producer.h>
#include <unistd.h>
#include "mainwindow.hpp"
#include "bkk_elapsed_timer.hpp"

int main(int argc, char *argv[])
{
    rbuflogd_producer_t loggerProducer {};

    QApplication app(argc, argv);

    const bool loggerReady = rbuflogd_producer_open(&loggerProducer, "MainApp") == 0;
    if (loggerReady) {
        rbuflogd_producer_log(&loggerProducer, RBUF_LOG_LEVEL_INFO, "startup", "Application started");
    }

    MainWindow window;
    window.show();

    const int exitCode = app.exec();
    if (loggerReady) {
        rbuflogd_producer_log(
            &loggerProducer,
            RBUF_LOG_LEVEL_INFO,
            "shutdown",
            QString("Application exiting with code %1").arg(exitCode).toStdString().c_str());
        rbuflogd_producer_close(&loggerProducer);
    }

    return exitCode;
}
