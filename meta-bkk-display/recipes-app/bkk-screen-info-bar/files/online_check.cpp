#include "online_check.hpp"
#include <curl/curl.h>
#include <rbuflogd/logger.h>
#include <bkk_utils/bkk_utils_online_status.h>
#include <netinet/in.h> 
#include <string> 

namespace online_check {

void online_check_init() {
  curl_global_init(CURL_GLOBAL_DEFAULT);
}

bool is_online() {

  CURL* curl = curl_easy_init();
  if (!curl) {
    log_error("OnlineCheck", "Failed to initialize CURL");
    return false; // Initialization failed
  }

  // ping google: 

  curl_easy_setopt(curl, CURLOPT_URL, 
      "https://clients3.google.com/generate_204");
  curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  CURLcode result = curl_easy_perform(curl);

  long responseCode = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

  curl_easy_cleanup(curl);

  return (result == CURLE_OK && responseCode == 204) ? true : false;
}


int get_ip_address(std::string &ip_address) {
  char ip_buffer[INET_ADDRSTRLEN] = {0};
  const ip_add_status_t ip_status = fetch_ip_addr(
    "wlan0", ip_buffer);

  if (ip_status != IP_ADD_STATUS_HAS_IP) {
    log_error("OnlineCheck", "Failed to retrieve IP address");
    return -1; // Error retrieving IP address
  }

  ip_address = std::string(ip_buffer);
  return 0; // Success
}

} // namespace online_check