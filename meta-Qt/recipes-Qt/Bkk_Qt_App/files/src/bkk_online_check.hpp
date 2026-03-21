#ifndef BKK_ONLINE_CHECK_HPP
#define BKK_ONLINE_CHECK_HPP

#include <cstdint>
#include <mutex>

struct OnlineChecker {
    OnlineChecker(); 
    ~OnlineChecker();

    bool isOnline() const;
    void cyclicCheck();

private:
    bool onlineStatus;
    mutable std::mutex statusMutex;
    uint8_t errorCode;

    void setOnlineStatus(bool status) {
        std::lock_guard<std::mutex> lock(statusMutex);
        onlineStatus = status;
    }
};

#endif // BKK_ONLINE_CHECK_HPP