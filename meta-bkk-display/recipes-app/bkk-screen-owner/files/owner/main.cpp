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

  int start_thread_res = screen.start_receive_thread();
  if (start_thread_res != 0) {
    log_error("Init", "Failed to start receive thread");
    return 1;
  }

  start_thread_res = screen.start_alive_check_thread();
  if (start_thread_res != 0) {
    log_error("Init", "Failed to start alive check thread");
    return 1; 
  }

  screen.show();

  const int exitCode = app.exec();
  log_error("Exit", ("Application exited with code: " 
    + std::to_string(exitCode)).c_str());

  rbuflogd_logger_close();
  

  return exitCode;
}
