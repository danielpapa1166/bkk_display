#include "bkk_api_worker.hpp"
#include "bkk_elapsed_timer.hpp"
#include "bkk_logger.hpp"

#include <QString>
#include <array>
#include <cstring>
#include <exception>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

namespace {

const char *getStationName(const char *stationId) {
  if (std::strcmp(stationId, "F02261") == 0) {
    return "Hollókő utca";
  }

  if (std::strcmp(stationId, "F02122") == 0) {
    return "Diószegi út";
  }

  return stationId;
}

} // namespace

BkkApiWorker::BkkApiWorker(QObject *parent) 
    : QThread(parent), 
    lastFetchDurationMs(0), 
    errorCode(BkkApiError::None) {

  loadStationList();
  (void) ensureApi();
  arrivals.clear();
}

bool BkkApiWorker::ensureApi() {
  if (!api) {
    try {
      api = std::make_unique<BkkApi>();
    } 
    catch (const std::exception& ex) {
      Logger::error("BkkApiWorker", QString("Error initializing BkkApi: %1").arg(ex.what()));
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

void BkkApiWorker::requestFetch() {
  fetchRequested.store(true);
}


void BkkApiWorker::run() {
  while (!isInterruptionRequested()) {
    if (fetchRequested.exchange(false)) { 
      // new request: perform fetch: 
      fetchData();

      // emit signal for the parent: 
      emit fetchCompleted();
    }

    msleep(100);
  }
}


void BkkApiWorker::fetchData() {
  if (!ensureApi()) {
    std::lock_guard<std::mutex> lock(arrivalsMutex);
    errorCode = BkkApiError::ApiUninitialized;
    lastFetchDurationMs = 0;
    return;
  }

  BkkElapsedTimer totalTimer;

  std::vector<StationArrival> mergedArrivals;
  bool fetchedAny = false;

  for (const auto &stationId : stationIdList) {
    const char *stationIdCStr = stationId.c_str();
    try {
      BkkElapsedTimer stationTimer;
      auto stationArrivals = api->get_arrivals_for_station(stationIdCStr);
      const auto stationMs = stationTimer.elapsedMs();
      for (auto &arrival : stationArrivals) {
        StationArrival stationArrival;
        stationArrival.arrival = std::move(arrival);
        stationArrival.station_id = stationId;
        stationArrival.station_name = getStationName(stationIdCStr);
        mergedArrivals.push_back(std::move(stationArrival));
      }

      Logger::info(
        "BkkApiWorker",
        QString("Fetched station %1 in %2 ms (%3 arrivals)")
          .arg(stationIdCStr)
          .arg(stationMs)
          .arg(static_cast<int>(stationArrivals.size())));
      fetchedAny = true;
    } 
    catch (const std::exception &ex) {
      Logger::warning(
        "BkkApiWorker",
        QString("Error fetching arrivals for station %1: %2")
          .arg(stationIdCStr)
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
    "BkkApiWorker",
    QString("Fetch cycle completed in %1 ms (status=%2, arrivals=%3)")
      .arg(totalMs)
      .arg(errorCode == BkkApiError::None ? "ok" : "failed")
      .arg(static_cast<int>(arrivalsCount)));
}

void BkkApiWorker::loadStationList() {
  static constexpr const char *configPath = "/etc/bkk-api/config.json";

  std::ifstream file(configPath);
  if (!file.is_open()) {
    Logger::warning("BkkApiWorker",
      QString("Cannot open config file: %1").arg(configPath));
    return;
  }

  try {
    nlohmann::json config = nlohmann::json::parse(file);

    if (config.contains("stations") && config["stations"].is_array()) {
      stationIdList.clear();
      for (const auto &station : config["stations"]) {
        if (station.is_string()) {
          stationIdList.push_back(station.get<std::string>());
        }
      }
      Logger::info("BkkApiWorker",
        QString("Loaded %1 station(s) from config")
          .arg(static_cast<int>(stationIdList.size())));
    } 
    else {
      Logger::warning("BkkApiWorker",
        "Config file missing 'stations' array");
    }
  } 
  catch (const std::exception &ex) {
    Logger::error("BkkApiWorker",
      QString("Error parsing config file: %1").arg(ex.what()));
  }
}

std::vector<StationArrival> BkkApiWorker::getArrivals() const {
  std::lock_guard<std::mutex> lock(arrivalsMutex);
  return arrivals;
}

uint64_t BkkApiWorker::getLastFetchDurationMs() const {
  std::lock_guard<std::mutex> lock(arrivalsMutex);
  return lastFetchDurationMs;
}

BkkApiError BkkApiWorker::getErrorCode() const {
  std::lock_guard<std::mutex> lock(arrivalsMutex);
  return errorCode;
}
