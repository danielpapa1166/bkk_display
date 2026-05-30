#ifndef BKK_ONLINE_CHECK_HPP
#define BKK_ONLINE_CHECK_HPP

#include <cstdint>
#include <mutex>

#include <rbuflogd/producer.h>

struct OnlineChecker {
    OnlineChecker(); 
    ~OnlineChecker();

    bool isOnline() const;
    uint64_t getLastResponseTimeMs() const;
    void cyclicCheck();

private:
    bool onlineStatus;
    mutable std::mutex statusMutex;
    uint8_t errorCode;
    uint64_t lastResponseTimeMs;

    void setOnlineStatus(bool status) {
        std::lock_guard<std::mutex> lock(statusMutex);
        onlineStatus = status;
    }

    rbuflogd_producer_t loggerProducer {};
};

#endif // BKK_ONLINE_CHECK_HPP