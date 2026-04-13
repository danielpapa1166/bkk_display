#include "bkk_info_bar_handler.hpp"

#include <QHBoxLayout>
#include <QPixmap>

BkkInfoBarHandler::BkkInfoBarHandler(QWidget *statRow) 
    : QObject(statRow), statusBarRow(statRow) {

  setupUi();
  startWorkerThread();
  startTimers();
}

BkkInfoBarHandler::~BkkInfoBarHandler() {
  stopTimers();
  stopWorkerThread();
}

void BkkInfoBarHandler::setupUi() {
  auto *statusRowLayout = new QHBoxLayout(statusBarRow);
  statusRowLayout->setContentsMargins(0, 0, 0, 0);
  statusRowLayout->setSpacing(8);

  clockLabel = new QLabel(statusBarRow);
  clockLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  clockLabel->setFixedWidth(86);

  bkkLogoLabel = new QLabel(statusBarRow);
  bkkLogoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  bkkLogoLabel->setFixedSize(106, 40);
  statusRowLayout->addWidget(bkkLogoLabel);

  statusRowLayout->addStretch(1);

  wifiIconLabel = new QLabel(statusBarRow);
  wifiIconLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  wifiIconLabel->setFixedSize(40, 40);
  statusRowLayout->addWidget(wifiIconLabel);

  statusRowLayout->addSpacing(12);

  statusRowLayout->addWidget(clockLabel);
}

void BkkInfoBarHandler::startWorkerThread() {
  QObject::connect(&workerThread, &WorkerThread::clockUpdateCompleted,
                   this, &BkkInfoBarHandler::handleClockUpdateCompleted);
  QObject::connect(&workerThread, &WorkerThread::onlineCheckCompleted,
                   this, &BkkInfoBarHandler::handleOnlineCheckCompleted);

  if (!workerThread.isRunning()) {
    workerThread.start();
  }
}

void BkkInfoBarHandler::stopWorkerThread() {
  if (!workerThread.isRunning()) {
    return;
  }
  workerThread.requestInterruption();
  workerThread.wait();
}

void BkkInfoBarHandler::startTimers() {
  workerThread.requestClockUpdate();
  workerThread.requestOnlineCheck();

  QObject::connect(&clockUpdateTimer, &QTimer::timeout, this, [this]() {
    workerThread.requestClockUpdate();
  });
  clockUpdateTimer.start(1000);

  QObject::connect(&onlineCheckTimer, &QTimer::timeout, this, [this]() {
    workerThread.requestOnlineCheck();
  });
  onlineCheckTimer.start(5000);

  updateUi();
}

void BkkInfoBarHandler::stopTimers() {
  onlineCheckTimer.stop();
  clockUpdateTimer.stop();
}

void BkkInfoBarHandler::handleClockUpdateCompleted() {
  clockText = workerThread.getClockText();
  updateUi();
}

void BkkInfoBarHandler::handleOnlineCheckCompleted() {
  onlineStatus = workerThread.isOnline();
  updateUi();
}

void BkkInfoBarHandler::updateUi() {
  clockLabel->setText(QString::fromStdString(clockText));

  wifiIconLabel->setPixmap(
    QPixmap(onlineStatus ? ":/icons/wifi_on.png" : ":/icons/wifi_off.png")
      .scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));

  bkkLogoLabel->setPixmap(
    QPixmap(":/icons/bkk_logo.png")
      .scaled(106, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}