#include "worker_thread.hpp"

#include <QMutexLocker>

WorkerThread::WorkerThread(QObject *parent)
    : QThread(parent),
      errorCode(BkkApiError::None),
      fetchRequested(false)
{
}

void WorkerThread::requestFetch()
{
    fetchRequested.store(true);
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

void WorkerThread::run()
{
    while (!isInterruptionRequested()) {
        if (fetchRequested.exchange(false)) {
            // if fetch requested: 
            apiWrapper.fetchData();

            {
                QMutexLocker lock(&resultMutex);
                arrivals = apiWrapper.getArrivals();
                errorCode = apiWrapper.getErrorCode();
            }

            // signal main thread that new data is available
            fetchCompleted();
            continue;
        }

        QThread::msleep(250);
    }
}