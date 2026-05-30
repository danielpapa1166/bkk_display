#ifndef BKK_CLOCK_UPDATE_HPP
#define BKK_CLOCK_UPDATE_HPP

#include <cstdint>
#include <mutex>
#include <string>

struct CLockUpdater {
    CLockUpdater(); 
    ~CLockUpdater();

    std::string getClock() const;
    void update();

private:

    mutable std::mutex clockMutex;
    std::string clockText;
    uint8_t errorCode;


};

#endif // BKK_CLOCK_UPDATE_HPP