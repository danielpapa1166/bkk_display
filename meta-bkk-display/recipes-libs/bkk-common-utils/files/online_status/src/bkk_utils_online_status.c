#include "bkk_utils_online_status.h"
#include <curl/curl.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


static int init_done_flag = 0;

online_status_t is_online() {
  if(init_done_flag == 0) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    init_done_flag = 1;
  }

   CURL* curl = curl_easy_init();
  if (!curl) {
    return ONLINE_STATUS_UNKNOWN; // Initialization failed
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

  return (result == CURLE_OK && responseCode == 204) 
    ? ONLINE_STATUS_ONLINE : ONLINE_STATUS_OFFLINE;

}



ip_add_status_t fetch_ip_addr(const char *interface_name, 
    char address[INET_ADDRSTRLEN]) {
  
  if(address == NULL || interface_name == NULL) {
    return IP_ADD_STATUS_UNKNOWN;
  }

  struct ifaddrs *interfaces = NULL;
  const int getifa_res = getifaddrs(&interfaces);
  if (getifa_res != 0) {
    return IP_ADD_STATUS_UNKNOWN;
  }

  bool found = false;

  for (struct ifaddrs *ifa = interfaces; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL) {
      continue;
    }

    if (ifa->ifa_addr->sa_family == AF_INET &&
        strcmp(ifa->ifa_name, interface_name) == 0) {
      struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
      const char * inet_res = inet_ntop(
        AF_INET, 
        &(sa->sin_addr), 
        address, 
        INET_ADDRSTRLEN);

      if(inet_res != NULL) {
        found = true;
      }
      break;
    }
  }

  freeifaddrs(interfaces);
  return found ? IP_ADD_STATUS_HAS_IP : IP_ADD_STATUS_NO_IP;

}