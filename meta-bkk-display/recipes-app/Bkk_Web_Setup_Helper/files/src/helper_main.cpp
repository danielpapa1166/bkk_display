#include <QApplication>
#include <unistd.h>
#include "helper_mainwindow.hpp"

int main(int argc, char *argv[])
{

    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    const int exitCode = app.exec();
    return exitCode;
}
