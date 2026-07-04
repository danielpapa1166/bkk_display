#include <QApplication>
#include <rbuflogd/logger.h>
#include <unistd.h>
#include "screen.hpp"

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

  screen.show();

  const int exitCode = app.exec();
  log_error("Exit", ("Application exited with code: " 
    + std::to_string(exitCode)).c_str());

  rbuflogd_logger_close();
  

  return exitCode;
}
