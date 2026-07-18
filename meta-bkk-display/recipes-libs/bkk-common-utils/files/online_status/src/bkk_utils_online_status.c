#include "bkk_utils_online_status.h"
#include <curl/curl.h>


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