#include "bkk_api_wrapper.hpp"
#include "bkk_elapsed_timer.hpp"
#include "bkk_logger.hpp"

#include <QString>
#include <array>
#include <cstring>
#include <exception>
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

BkkApiWrapper::BkkApiWrapper() : lastFetchDurationMs(0), errorCode(BkkApiError::None) {
    (void) ensureApi();
    arrivals.clear();
}

BkkApiWrapper::~BkkApiWrapper() = default;

bool BkkApiWrapper::ensureApi() {
	if (!api) {
        try {
            api = std::make_unique<BkkApi>();
        } catch (const std::exception& ex) {
            Logger::error("BkkApiWrapper", QString("Error initializing BkkApi: %1").arg(ex.what()));
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
        std::lock_guard<std::mutex> lock(arrivalsMutex);
        lastFetchDurationMs = 0;
        return;
	}

    BkkElapsedTimer totalTimer;

    std::vector<StationArrival> mergedArrivals;
    bool fetchedAny = false;
    static constexpr std::array<const char *, 2> stationIds = {"F02261", "F02122"};

    for (const char *stationId : stationIds) {
        try {
            BkkElapsedTimer stationTimer;
            auto stationArrivals = api->get_arrivals_for_station(stationId);
            const auto stationMs = stationTimer.elapsedMs();
            for (auto &arrival : stationArrivals) {
                StationArrival stationArrival;
                stationArrival.arrival = std::move(arrival);
                stationArrival.station_id = stationId;
                stationArrival.station_name = getStationName(stationId);
                mergedArrivals.push_back(std::move(stationArrival));
            }

            Logger::info(
                "BkkApiWrapper",
                QString("Fetched station %1 in %2 ms (%3 arrivals)")
                    .arg(stationId)
                    .arg(stationMs)
                    .arg(static_cast<int>(stationArrivals.size())));
            fetchedAny = true;
        } catch (const std::exception &ex) {
            Logger::warning(
                "BkkApiWrapper",
                QString("Error fetching arrivals for station %1: %2")
                    .arg(stationId)
                    .arg(ex.what()));
        }
    }

    const auto totalMs = totalTimer.elapsedMs();

    size_t arrivalsCount = 0;
    {
        std::lock_guard<std::mutex> lock(arrivalsMutex);
        arrivals = std::move(mergedArrivals);
        lastFetchDurationMs = totalMs;
        errorCode = fetchedAny ? BkkApiError::None : BkkApiError::FetchFailed;
        arrivalsCount = arrivals.size();
    }

    Logger::info(
        "BkkApiWrapper",
        QString("Fetch cycle completed in %1 ms (status=%2, arrivals=%3)")
            .arg(totalMs)
            .arg(errorCode == BkkApiError::None ? "ok" : "failed")
            .arg(static_cast<int>(arrivalsCount)));
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

uint64_t BkkApiWrapper::getLastFetchDurationMs() const {
    std::lock_guard<std::mutex> lock(arrivalsMutex);
    return lastFetchDurationMs;
}
