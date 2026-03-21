#include "bkk_api_wrapper.hpp"

#include <array>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>

namespace {

const char *getStationName(const char *stationId)
{
    if (std::strcmp(stationId, "F02261") == 0) {
        return "Hollókő utca";
    }

    if (std::strcmp(stationId, "F02122") == 0) {
        return "Diószegi út";
    }

    return stationId;
}

} // namespace

BkkApiWrapper::BkkApiWrapper() : errorCode(BkkApiError::None) {
    (void) ensureApi();
    arrivals.clear();
}

BkkApiWrapper::~BkkApiWrapper() = default;

bool BkkApiWrapper::ensureApi() {
	if (!api) {
        try {
            api = std::make_unique<BkkApi>();
        } catch (const std::exception& ex) {
            std::cerr << "Error initializing BkkApi: " 
                << ex.what() << std::endl;
            api.reset();
            errorCode = BkkApiError::InitializationFailed;
            return false;
        }
	}

    if(!api) {
        errorCode = BkkApiError::InitializationFailed;
        return false;
    }
    else {
        errorCode = BkkApiError::None;
        return true;
    }
}

void BkkApiWrapper::fetchData() {
	if (!ensureApi()) {
        errorCode = BkkApiError::ApiUninitialized;
        return;
	}

    std::vector<StationArrival> mergedArrivals;
    bool fetchedAny = false;
    static constexpr std::array<const char *, 2> stationIds = {"F02261", "F02122"};

    for (const char *stationId : stationIds) {
        try {
            auto stationArrivals = api->get_arrivals_for_station(stationId);
            for (auto &arrival : stationArrivals) {
                StationArrival stationArrival;
                stationArrival.arrival = std::move(arrival);
                stationArrival.station_id = stationId;
                stationArrival.station_name = getStationName(stationId);
                mergedArrivals.push_back(std::move(stationArrival));
            }
            fetchedAny = true;
        } catch (const std::exception &ex) {
            std::cerr << "Error fetching arrivals for station " << stationId
                      << ": " << ex.what() << std::endl;
        }
    }

    std::lock_guard<std::mutex> lock(arrivalsMutex);
    arrivals = std::move(mergedArrivals);
    errorCode = fetchedAny ? BkkApiError::None : BkkApiError::FetchFailed;
}

std::vector<StationArrival> BkkApiWrapper::getArrivals() {
    std::lock_guard<std::mutex> lock(arrivalsMutex);
    return arrivals;
}

std::string BkkApiWrapper::getArrivalsText() {
    std::lock_guard<std::mutex> lock(arrivalsMutex);
    if (arrivals.empty()) {
        return "No arrivals";
    }
    std::string out;
    for (const auto& a : arrivals) {
        out += a.station_name
            + "  |  " + a.arrival.line
            + "  →  " + a.arrival.destination
            + "  |  " + a.arrival.departure_time
            + "  (" + std::to_string(a.arrival.departs_in_min) + " min)"
            + "  [" + a.station_id + "]\n";
    }
    return out;
}
