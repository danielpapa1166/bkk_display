#ifndef WORKER_THREAD_HPP
#define WORKER_THREAD_HPP

#include <QMutex>
#include <QThread>

#include <atomic>

#include "bkk_api_wrapper.hpp"

class WorkerThread : public QThread
{
    Q_OBJECT

public:
    explicit WorkerThread(QObject *parent = nullptr);

    void requestFetch();
    std::vector<StationArrival> getArrivals();
    BkkApiError getErrorCode() const;

signals:
    void fetchCompleted();

protected:
    void run() override;

private:
    mutable QMutex resultMutex;
    BkkApiWrapper apiWrapper;
    std::vector<StationArrival> arrivals;
    BkkApiError errorCode;
    std::atomic<bool> fetchRequested;
};

#endif