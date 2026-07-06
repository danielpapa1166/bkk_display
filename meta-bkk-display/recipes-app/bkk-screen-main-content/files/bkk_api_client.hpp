#ifndef BKK_API_CLIENT_HPP
#define BKK_API_CLIENT_HPP

#include <bkk_uds/bkk_uds_client.h>
#include <bkk_uds/bkk_api_arrival.h>
#include <bkk_uds/bkk_stop_utils.h>
#include "bkk_screen_client/common_defs.hpp"
#include <string>
#include <vector>

namespace api_client {
  int load_api_key(std::string& api_key);
  int load_station_ids(
    std::vector<std::string>& stationIdList, 
    std::vector<std::string>& stationNameList
  );
  int fetch_arrivals(
    std::string& apiKey,
    std::vector<std::string>& stationIdList,
    std::vector<arrival_info_t>& arrivals
  );
}; // namespace api_client

#endif // BKK_API_CLIENT_HPP