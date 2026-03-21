#include <curl/curl.h>
#include "bkk_online_check.hpp"


OnlineChecker::OnlineChecker() : onlineStatus(false), errorCode(0) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

OnlineChecker::~OnlineChecker() {
    curl_global_cleanup();
}

bool OnlineChecker::isOnline() const {
    std::lock_guard<std::mutex> lock(statusMutex);
    return onlineStatus;
}

void OnlineChecker::cyclicCheck() {
    CURL* curl = curl_easy_init();
    if (!curl) {
        errorCode = 1; // Initialization failed
        setOnlineStatus(false); 
        return;
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

    setOnlineStatus(
        (result == CURLE_OK && responseCode == 204) ? true : false); 

    return; // Success
}
