#include "bkk_api_client.hpp"
#include "bkk_uds/bkk_uds_client.h"
#include "bkk_screen_client/common_defs.hpp"
#include "rbuflogd/logger.h"
#include "bkk_tee/bkk_tee_client.h"
#include "cJSON.h"
#include <algorithm>
#include <cstring>
#include <fstream>

namespace api_client {

static std::string bkk_client_status_to_string(bkk_client_status_t status) {
  switch (status) {
    case client_OK:
      return "OK";
    case client_InvalidArguments:
      return "Invalid Arguments";
    case client_SocketError:
      return "Socket Error";
    case client_ConnectionFailed:
      return "Connection Failed";
    case client_SendFailed:
      return "Send Failed";
    case client_ReceiveFailed:
      return "Receive Failed";
    default:
      return "Unknown Status";
  }
}


int load_api_key(std::string& api_key) {

  char key_buffer[BKK_TEE_MAX_OBJ_ID_LEN];
  size_t key_buffer_len = sizeof(key_buffer);
  bkk_tee_client_status_t teeRes = bkk_tee_get(
    tee_object_type_api_key, key_buffer, &key_buffer_len);

  if(teeRes != bkk_tee_client_err_none) {
    log_error("TEE Get", 
      ("Failed to retrieve API key from TEE, error code: " 
        + std::to_string(teeRes)).c_str()
    );
  }
  else {
    api_key = std::string(key_buffer, key_buffer_len);
    
    log_info("TEE Get", 
      ("Retrieved API key from TEE: " 
        + std::string(key_buffer, key_buffer_len)).c_str()
    );
  }

  return 0;
  
}


int load_station_ids(
    std::vector<std::string>& stationIdList, 
    std::vector<std::string>& stationNameList) {

  static constexpr const char *configPath = "/etc/bkk-api/config.json";

  std::ifstream file(configPath);
  if (!file.is_open()) {
    log_error("StopCfg", 
      ("Cannot open config file: " 
        + std::string(configPath)).c_str()
    );
    return -1;
  }

  const std::string content(
    (std::istreambuf_iterator<char>(file)),
    std::istreambuf_iterator<char>());

  cJSON *root = cJSON_Parse(content.c_str());
  if (!root) {
    const char *errPtr = cJSON_GetErrorPtr();
    log_error("StopCfg", 
      ("Failed to parse config JSON: " 
        + std::string(errPtr ? errPtr : "Unknown error")).c_str()
    );
    return -1;
  }

  const cJSON *stations = cJSON_GetObjectItemCaseSensitive(root, "stations");
  if (!cJSON_IsArray(stations)) {
    log_error("StopCfg", "Config file missing 'stations' array");
    cJSON_Delete(root);
    return -1;
  }

  stationIdList.clear();
  stationNameList.clear();
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
      log_warning("StopCfg", 
        ("Station ID not found in stop list: " + id).c_str()
      );
      stationNameList.push_back(id);
    }
  }

  cJSON_Delete(root);

  log_info("StopCfg", 
    ("Loaded " + std::to_string(stationIdList.size()) 
    + " station IDs from config").c_str()
  );

  return 0;
}


api_fetch_status_t fetch_arrivals(
    std::string& api_key,
    std::vector<std::string>& stationIdList,
    std::vector<std::string>& stationNameList,
    std::vector<arrival_info_t>& arrivals
  ) {

  api_fetch_status_t fetch_status = { 
    0, 
    "", 
    ""
  };

  bool fetchedAny = false;
  for (size_t i = 0; i < stationIdList.size(); i++) {
    
    const auto &stationId = stationIdList[i];
    const char *stationName = stationNameList[i].c_str();
    bkk_uds_request_t request {};
    bkk_uds_response_t response {};

    strncpy(request.api_key, api_key.c_str(), BKK_UDS_MAX_KEY_LEN - 1);
    strncpy(request.stop_id, stationId.c_str(), BKK_UDS_MAX_STOP_ID_LEN - 1);

    const int res = send_bkk_uds_query(&request, &response);
    if(res == 0) {

      if(response.status == bkk_api_status::Ok) {
        fetchedAny = true;
        for(int arrivalIdx = 0; arrivalIdx < response.number_of_arrivals; arrivalIdx++) {

          // copy fetched arrivals: 

          arrival_info_t arrivalInfo = { 0 };

          arrivalInfo.foo = 0;
          strncpy(
            arrivalInfo.station, 
            stationName, 
            BKK_SCREEN_STATION_NAME_MAX_LEN - 1
          );

          strncpy(
            arrivalInfo.line, 
            response.arrivals[arrivalIdx].line_id, 
            BKK_SCREEN_LINE_NAME_MAX_LEN - 1
          );

          arrivalInfo.vehicle_type = (int)response.arrivals[arrivalIdx].vehicle_type;


          strncpy(
            arrivalInfo.destination, 
            response.arrivals[arrivalIdx].destination, 
            BKK_SCREEN_DESTINATION_NAME_MAX_LEN - 1
          );

          arrivalInfo.departure_time = response.arrivals[arrivalIdx].departs_in_min;
          arrivals.push_back(arrivalInfo);

        }
      }
      else {
        fetch_status.any_error = 1;
        fetch_status.local_server_status = bkk_client_status_to_string((bkk_client_status_t)res);
        fetch_status.remote_server_status = error_code_to_string(response.status);

        return fetch_status;
      }
    } 
    else {
      // handle error: 
      log_error("API Fetch", (
        "Failed to fetch arrivals for station ID: " 
        + stationId 
        + ", error code: " 
        + std::to_string(res)).c_str()
      );

      fetch_status.any_error = 1;
      fetch_status.local_server_status = bkk_client_status_to_string((bkk_client_status_t)res);
      fetch_status.remote_server_status = error_code_to_string(response.status);

      return fetch_status;
    }
  }

  if(fetchedAny) {
    std::sort(arrivals.begin(), arrivals.end(), 
      [](const arrival_info_t &left, const arrival_info_t &right) {
      // sort by departure time
      return left.departure_time < right.departure_time;
    });
  } 
  else {
    log_warning("API Fetch", "No arrival data fetched from API");
  }
  
  return fetch_status;
}
} // namespace api_client