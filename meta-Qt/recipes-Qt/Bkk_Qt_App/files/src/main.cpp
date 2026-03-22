#include <QApplication>
#include <unistd.h>
#include "bkk_logger.hpp"
#include "mainwindow.hpp"
#include "bkk_elapsed_timer.hpp"

int main(int argc, char *argv[])
{

    uint64_t elapsed_ms = BkkElapsedTimer::measureMs([&]() {
        sleep(1); // Simulate some startup work
    });

    QApplication app(argc, argv);

    if (!Logger::init("bkk-qt-app")) {
        Logger::warning("startup", "Logger file initialization failed, falling back to stderr only");
    }

    
    Logger::setMinLevel(Logger::Level::Info);
    Logger::info("startup", "Application started");
    
    Logger::info("startup", QString("Elapsed timer test run took %1 ms").arg(elapsed_ms)); 
    MainWindow window;
    window.show();

    const int exitCode = app.exec();
    Logger::info("shutdown", QString("Application exiting with code %1").arg(exitCode));
    Logger::shutdown();
    return exitCode;
}
