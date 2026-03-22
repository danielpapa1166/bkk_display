#ifndef BKK_API_WRAPPER_HPP
#define BKK_API_WRAPPER_HPP

#include <bkk_api/bkk_api.hpp>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>


enum class BkkApiError {
    None = 0,
    ApiUninitialized, 
    InitializationFailed,
    FetchFailed
};

struct StationArrival {
    Arrival arrival;
    std::string station_id;
    std::string station_name;
};


struct BkkApiWrapper {
    BkkApiWrapper();
    ~BkkApiWrapper();

    void fetchData();
    std::vector<StationArrival> getArrivals();
    std::string getArrivalsText();
    uint64_t getLastFetchDurationMs() const;
    BkkApiError getErrorCode() const {
        return errorCode;
    }



private:
    bool ensureApi();

    std::unique_ptr<BkkApi> api = nullptr;

    mutable std::mutex arrivalsMutex;
    std::vector<StationArrival> arrivals;
    uint64_t lastFetchDurationMs;

    BkkApiError errorCode;
}; 


#endif // BKK_API_WRAPPER_HPP