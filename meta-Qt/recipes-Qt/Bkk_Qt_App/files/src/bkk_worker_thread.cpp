#include "bkk_worker_thread.hpp"

#include <QMutexLocker>

WorkerThread::WorkerThread(QObject *parent)
    : QThread(parent),
      errorCode(BkkApiError::None),
      clockText(),
      onlineStatus(false),
      apiFetchRequested(false),
      clockUpdateRequested(false),
      onlineCheckRequested(false)
{
}

void WorkerThread::requestApiFetch()
{
    apiFetchRequested.store(true);
}

void WorkerThread::requestClockUpdate()
{
    clockUpdateRequested.store(true);
}

void WorkerThread::requestOnlineCheck()
{
    onlineCheckRequested.store(true);
}

std::vector<StationArrival> WorkerThread::getArrivals()
{
    QMutexLocker lock(&resultMutex);
    return arrivals;
}

BkkApiError WorkerThread::getErrorCode() const
{
    QMutexLocker lock(&resultMutex);
    return errorCode;
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

        if (apiFetchRequested.exchange(false)) {
            apiWrapper.fetchData();

            {
                QMutexLocker lock(&resultMutex);
                arrivals = apiWrapper.getArrivals();
                errorCode = apiWrapper.getErrorCode();
            }

            apiFetchCompleted();
            didWork = true;
        }

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