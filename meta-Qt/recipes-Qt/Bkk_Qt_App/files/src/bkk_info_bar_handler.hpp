#ifndef BKK_INFO_BAR_HANDLER_HPP
#define BKK_INFO_BAR_HANDLER_HPP

#include <QLabel>
#include <QObject>
#include <QTimer>
#include <QWidget>

#include <string>

#include "bkk_worker_thread.hpp"

class BkkInfoBarHandler : public QObject {
  Q_OBJECT
public:
  explicit BkkInfoBarHandler(QWidget *statusBarRow);
  ~BkkInfoBarHandler();

private:
  void setupUi();
  void startWorkerThread();
  void stopWorkerThread();
  void startTimers();
  void stopTimers();
  void handleClockUpdateCompleted();
  void handleOnlineCheckCompleted();
  void updateUi();

  QWidget *statusBarRow;
  QLabel *clockLabel = nullptr;
  QLabel *bkkLogoLabel = nullptr;
  QLabel *wifiIconLabel = nullptr;

  WorkerThread workerThread;

  std::string clockText;
  bool onlineStatus = false;

  QTimer clockUpdateTimer;
  QTimer onlineCheckTimer;
}; 


#endif // BKK_INFO_BAR_HANDLER_HPP