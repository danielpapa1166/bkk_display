#include <QApplication>
#include <rbuflogd/logger.h>
#include <unistd.h>
#include "screen.hpp"

// start with: 
// QT_QPA_PLATFORM=eglfs QT_QPA_EGLFS_HIDECURSOR=1 /usr/bin/bkk-screen-owner # -platform eglfs

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  rbuflogd_logger_init("ScrOwner");

  BkkScreen screen(nullptr);

  QCursor cursor(Qt::BlankCursor);
  QApplication::setOverrideCursor(cursor);
  QApplication::changeOverrideCursor(cursor);


  screen.show();

  const int exitCode = app.exec();
  log_error("Exit", ("Application exited with code: " 
    + std::to_string(exitCode)).c_str());

  rbuflogd_logger_close();
  

  return exitCode;
}
