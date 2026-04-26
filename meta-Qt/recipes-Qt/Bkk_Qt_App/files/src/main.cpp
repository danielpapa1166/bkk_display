#include <QApplication>
#include <rbuflogd/producer.h>
#include <unistd.h>
#include "mainwindow.hpp"
#include "bkk_elapsed_timer.hpp"

int main(int argc, char *argv[])
{
    rbuflogd_producer_t loggerProducer {};

    uint64_t elapsed_ms = BkkElapsedTimer::measureMs([&]() {
        sleep(1); // Simulate some startup work
    });

    QApplication app(argc, argv);

    const bool loggerReady = rbuflogd_producer_open(&loggerProducer, "MainApp") == 0;
    if (loggerReady) {
        rbuflogd_producer_log(&loggerProducer, RBUF_LOG_LEVEL_INFO, "startup", "Application started");
        rbuflogd_producer_log(
            &loggerProducer,
            RBUF_LOG_LEVEL_INFO,
            "startup",
            QString("Elapsed timer test run took %1 ms").arg(elapsed_ms).toStdString().c_str());
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
