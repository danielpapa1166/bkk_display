#include <QApplication>
#include <rbuflogd/logger.h>
#include <unistd.h>
#include "screen.hpp"

int main(int argc, char *argv[])
{
  rbuflogd_producer_t loggerProducer {};



  QApplication app(argc, argv);
  rbuflogd_logger_init("ScrOwner");

  BkkScreen screen(nullptr);
  screen_error_t error = screen.expose_screen_components();
  if (error != BKK_SCREEN_ERROR_NONE) {
    log_error("Init", 
      ("Failed to expose screen components. Error code: " 
        + std::to_string(error)).c_str()
    );
    rbuflogd_logger_close(); 
    return -1;
  }

  screen.show();

  const int exitCode = app.exec();
  log_error("Exit", ("Application exited with code: " 
    + std::to_string(exitCode)).c_str());

  rbuflogd_logger_close();
  

  return exitCode;
}
