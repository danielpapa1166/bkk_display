#include <QApplication>
#include <rbuflogd/producer.h>
#include <unistd.h>
#include "mainwindow.hpp"
#include "bkk_elapsed_timer.hpp"

static int arg_parser(int argc, char *argv[], const char * api_key_path);

int main(int argc, char *argv[])
{
    rbuflogd_producer_t loggerProducer {};

    char apiKeyPath[256]; 
    const int argResult = arg_parser(argc, argv, apiKeyPath);
    if (argResult != 0) {
        return 1;
    }  

    QApplication app(argc, argv);

    const bool loggerReady = rbuflogd_producer_open(&loggerProducer, "MainApp") == 0;
    if (loggerReady) {
        rbuflogd_producer_log(
            &loggerProducer, 
            RBUF_LOG_LEVEL_INFO, 
            "startup", 
            "Application started");
    }

    MainWindow window(nullptr, apiKeyPath);
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


static int arg_parser(int argc, char *argv[], const char * api_key_path) {
    int opt;
    while ((opt = getopt(argc, argv, "k:")) != -1) {
        switch (opt) {
            case 'k':
                strncpy(const_cast<char *>(api_key_path), optarg, 255);
                break;
            default:
                fprintf(stderr, "Usage: %s [-k api_key_path]\n", argv[0]);
                return -1;
        }
    }
    return 0;

}