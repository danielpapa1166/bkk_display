#include "bkk_api_client.hpp"
#include <bkk_uds/bkk_uds_client.h>
#include "bkk_screen_client/common_defs.hpp"
#include <cstring>

namespace api_client {


int fetch_arrivals(
    std::string& apiKey,
    std::vector<std::string>& stationIdList,
    std::vector<arrival_info_t>& arrivals
  ) {


  bool fetchedAny = false;
  for (size_t i = 0; i < stationIdList.size(); i++) {
    
    const auto &stationId = stationIdList[i];
    const char *stationName = stationIdList[i].c_str(); // todo get station name 
    bkk_uds_request_t request {};
    bkk_uds_response_t response {};

    strncpy(request.api_key, apiKey.c_str(), BKK_UDS_MAX_KEY_LEN - 1);
    strncpy(request.stop_id, stationId.c_str(), BKK_UDS_MAX_STOP_ID_LEN - 1);

    const int res = send_bkk_uds_query(&request, &response);
    if(res == 0) {
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


        strncpy(
          arrivalInfo.destination, 
          response.arrivals[arrivalIdx].destination, 
          BKK_SCREEN_DESTINATION_NAME_MAX_LEN - 1
        );

        arrivalInfo.departure_time = response.arrivals[arrivalIdx].departs_in_min;
        arrivals.push_back(arrivalInfo);

        printf(
          "Fetched arrival: station=%s, line=%s, destination=%s, departure_time=%d\n", 
          arrivalInfo.station, 
          arrivalInfo.line, 
          arrivalInfo.destination, 
          arrivalInfo.departure_time
        );
      }
    } 
    else {

    }
  }
  return fetchedAny;


}
}; // namespace api_client