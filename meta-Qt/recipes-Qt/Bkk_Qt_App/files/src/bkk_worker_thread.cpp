#include "bkk_worker_thread.hpp"

#include <QMutexLocker>

WorkerThread::WorkerThread(QObject *parent)
    : QThread(parent),
      clockText(),
      onlineStatus(false),
      clockUpdateRequested(false),
      onlineCheckRequested(false)
{
}

void WorkerThread::requestClockUpdate()
{
    clockUpdateRequested.store(true);
}

void WorkerThread::requestOnlineCheck()
{
    onlineCheckRequested.store(true);
}


std::string WorkerThread::getClockText() const
{
    QMutexLocker lock(&resultMutex);
    return clockText;
}

bool WorkerThread::isOnline() const
{
    QMutexLocker lock(&resultMutex);
    return onlineStatus;
}

void WorkerThread::run()
{
    while (!isInterruptionRequested()) {
        bool didWork = false;

        if (clockUpdateRequested.exchange(false)) {
            clockUpdater.update();

            {
                QMutexLocker lock(&resultMutex);
                clockText = clockUpdater.getClock();
            }

            clockUpdateCompleted();
            didWork = true;
        }

        if (onlineCheckRequested.exchange(false)) {
            onlineChecker.cyclicCheck();

            {
                QMutexLocker lock(&resultMutex);
                onlineStatus = onlineChecker.isOnline();
            }

            onlineCheckCompleted();
            didWork = true;
        }

        if (!didWork) {
            QThread::msleep(50);
        }
    }
}