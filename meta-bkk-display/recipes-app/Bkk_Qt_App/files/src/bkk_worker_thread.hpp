#ifndef BKK_WORKER_THREAD_HPP
#define BKK_WORKER_THREAD_HPP

#include <QMutex>
#include <QThread>

#include <atomic>
#include <string>

#include "bkk_clock_update.hpp"
#include "bkk_online_check.hpp"

class WorkerThread : public QThread
{
    Q_OBJECT

public:
    explicit WorkerThread(QObject *parent = nullptr);

    void requestClockUpdate();
    void requestOnlineCheck();

    std::string getClockText() const;
    bool isOnline() const;

signals:
    void clockUpdateCompleted();
    void onlineCheckCompleted();

protected:
    void run() override;

private:
    mutable QMutex resultMutex;

    CLockUpdater clockUpdater;
    OnlineChecker onlineChecker;

    std::string clockText;
    bool onlineStatus;

    std::atomic<bool> clockUpdateRequested;
    std::atomic<bool> onlineCheckRequested;
};

#endif