#ifndef BKK_WORKER_THREAD_HPP
#define BKK_WORKER_THREAD_HPP

#include <QMutex>
#include <QThread>

#include <atomic>
#include <string>

#include "bkk_api_wrapper.hpp"
#include "bkk_clock_update.hpp"
#include "bkk_online_check.hpp"

class WorkerThread : public QThread
{
    Q_OBJECT

public:
    explicit WorkerThread(QObject *parent = nullptr);

    void requestApiFetch();
    void requestClockUpdate();
    void requestOnlineCheck();

    std::vector<StationArrival> getArrivals();
    BkkApiError getErrorCode() const;
    std::string getClockText() const;
    bool isOnline() const;

signals:
    void apiFetchCompleted();
    void clockUpdateCompleted();
    void onlineCheckCompleted();

protected:
    void run() override;

private:
    mutable QMutex resultMutex;

    BkkApiWrapper apiWrapper;
    CLockUpdater clockUpdater;
    OnlineChecker onlineChecker;

    std::vector<StationArrival> arrivals;
    BkkApiError errorCode;
    std::string clockText;
    bool onlineStatus;

    std::atomic<bool> apiFetchRequested;
    std::atomic<bool> clockUpdateRequested;
    std::atomic<bool> onlineCheckRequested;
};

#endif