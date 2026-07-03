#include "online_check.hpp"
#include <curl/curl.h>

namespace online_check {

void online_check_init() {
  curl_global_init(CURL_GLOBAL_DEFAULT);
}

bool is_online() {

  CURL* curl = curl_easy_init();
  if (!curl) {
    printf("Failed to initialize CURL\n");
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

  printf("CURL result: %d\n", result);

  long responseCode = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

    printf("CURL response code: %ld\n", responseCode);

  curl_easy_cleanup(curl);

  return (result == CURLE_OK && responseCode == 204) ? true : false;
}

} // namespace online_check