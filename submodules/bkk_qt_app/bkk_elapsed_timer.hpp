#ifndef BKK_ELAPSED_TIMER_HPP
#define BKK_ELAPSED_TIMER_HPP

#include <chrono>
#include <cstdint>
#include <utility>

class BkkElapsedTimer {
public:
    BkkElapsedTimer() : startTime(std::chrono::steady_clock::now()) {}

    void restart()
    {
        startTime = std::chrono::steady_clock::now();
    }

    uint64_t elapsedMs() const
    {
        const auto elapsed = std::chrono::steady_clock::now() - startTime;
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
    }

    template <typename Func>
    static uint64_t measureMs(Func &&func)
    {
        BkkElapsedTimer timer;
        std::forward<Func>(func)();
        return timer.elapsedMs();
    }

private:
    std::chrono::steady_clock::time_point startTime;
};

#endif // BKK_ELAPSED_TIMER_HPP
