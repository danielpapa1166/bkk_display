#include "bkk_api_worker.hpp"
#include "bkk_elapsed_timer.hpp"
#include "bkk_uds/bkk_uds_protocol.h"
#include "bkk_uds/bkk_stop_utils.h"
#include "cJSON.h"
#include "rbuflogd/producer.h"
#include "rbuflogd/pub_common_types.h"

#include <QString>
#include <array>
#include <cstring>
#include <exception>
#include <fstream>
#include <string>



BkkApiWorker::BkkApiWorker(QObject *parent) 
    : QThread(parent), 
    lastFetchDurationMs(0), 
    errorCode(BkkApiError::None) {

  rbuflogd_producer_open(
    &loggerProducer, 
    "BkkApiWorker");

  rbuflogd_producer_log(
    &loggerProducer, 
    RBUF_LOG_LEVEL_DEBUG, 
    "Init", 
    "Init started"); 

  loadStationList();
  arrivals.clear();

}

BkkApiWorker::~BkkApiWorker() {
  rbuflogd_producer_close(&loggerProducer);
}

int BkkApiWorker::loadApiKey(const char * apiKeyPath) {
  char * key = nullptr; 
  const api_key_read_stat_t res = read_api_key_from_file(apiKeyPath, &key);

  if(res != API_KEY_READ_OK || key == nullptr) {
    rbuflogd_producer_log(
      &loggerProducer, 
      RBUF_LOG_LEVEL_ERROR, 
      "API Key", 
      QString("Failed to read API key from file: %1").arg(apiKeyPath).toStdString().c_str());
    return -1;
  }
  apiKey = key;
  free(key);
  return 0;
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
  BkkElapsedTimer totalTimer;

  std::vector<StationArrival> mergedArrivals;
  bool fetchedAny = false;
  bkk_uds_response_t response;
  for (size_t i = 0; i < stationIdList.size(); i++) {
    const auto &stationId = stationIdList[i];
    const char *stationName = stationNameList[i].c_str();
    bkk_uds_request_t request = { 0 };
    strncpy(request.api_key, apiKey.c_str(), BKK_UDS_MAX_KEY_LEN - 1);
    strncpy(request.stop_id, stationId.c_str(), BKK_UDS_MAX_STOP_ID_LEN - 1);

    const int res = send_bkk_uds_query(&request, &response);
    if(res == 0) {
      fetchedAny = true;
      for(int arrivalIdx = 0; arrivalIdx < response.number_of_arrivals; arrivalIdx++) {
        mergedArrivals.push_back(StationArrival{
          .arrival = response.arrivals[arrivalIdx],
          .station_id = stationId,
          .station_name = stationName
        });
      }
      rbuflogd_producer_log(
        &loggerProducer, 
        RBUF_LOG_LEVEL_INFO, 
        "fetch data", 
        QString("Fetched %1 arrivals for station_id %2")
        .arg(response.number_of_arrivals)
        .arg(stationId.c_str()).toStdString().c_str());
    } 
    else {
      rbuflogd_producer_log(
        &loggerProducer, 
        RBUF_LOG_LEVEL_ERROR, 
        "fetch data", 
        QString("Failed to fetch data for station_id %1")
        .arg(stationId.c_str()).toStdString().c_str());
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

  rbuflogd_producer_log(
    &loggerProducer, 
    RBUF_LOG_LEVEL_INFO, 
    "fetch data", 
    QString("Fetch cycle completed in %1 ms (status=%2, arrivals=%3)")
      .arg(totalMs)
      .arg(errorCode == BkkApiError::None ? "ok" : "failed")
      .arg(static_cast<int>(arrivalsCount)).toStdString().c_str());
}

void BkkApiWorker::loadStationList() {
  static constexpr const char *configPath = "/etc/bkk-api/config.json";

  std::ifstream file(configPath);
  if (!file.is_open()) {
    rbuflogd_producer_log(
      &loggerProducer, 
      RBUF_LOG_LEVEL_ERROR, 
      "config", 
      QString("Cannot open config file: %1").arg(configPath).toStdString().c_str());
    return;
  }

  const std::string content(
    (std::istreambuf_iterator<char>(file)),
    std::istreambuf_iterator<char>());

  cJSON *root = cJSON_Parse(content.c_str());
  if (!root) {
    const char *errPtr = cJSON_GetErrorPtr();
    rbuflogd_producer_log(
      &loggerProducer,
      RBUF_LOG_LEVEL_ERROR,
      "config",
      QString("Failed to parse config JSON%1")
        .arg(errPtr ? QString(": ") + errPtr : QString())
        .toStdString().c_str());
    return;
  }

  const cJSON *stations = cJSON_GetObjectItemCaseSensitive(root, "stations");
  if (!cJSON_IsArray(stations)) {
    rbuflogd_producer_log(
      &loggerProducer,
      RBUF_LOG_LEVEL_WARNING,
      "config",
      "Config file missing 'stations' array");
    cJSON_Delete(root);
    return;
  }

  stationIdList.clear();
  const cJSON *station = nullptr;
  cJSON_ArrayForEach(station, stations) {
    if (cJSON_IsString(station) && station->valuestring) {
      stationIdList.push_back(station->valuestring);
    }
  }

  for (const auto &id : stationIdList) {
    bkk_stop_t stop;
    if (find_stop_by_id(id.c_str(), &stop) == BKK_STOP_FOUND) {
      stationNameList.push_back(stop.stop_name);
    } 
    else {
      rbuflogd_producer_log(
        &loggerProducer,
        RBUF_LOG_LEVEL_WARNING,
        "config",
        ("Failed to find station name for id: " + id).c_str());
      stationNameList.push_back(id);
    }
  }

  cJSON_Delete(root);

  rbuflogd_producer_log(
    &loggerProducer,
    RBUF_LOG_LEVEL_INFO,
    "config",
    QString("Loaded %1 station(s) from config")
      .arg(static_cast<int>(stationIdList.size())).toStdString().c_str());
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
