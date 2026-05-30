#include <curl/curl.h>
#include <QString>
#include "bkk_elapsed_timer.hpp"
#include "bkk_online_check.hpp"


OnlineChecker::OnlineChecker() : onlineStatus(false), errorCode(0), lastResponseTimeMs(0) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    rbuflogd_producer_open(&loggerProducer, "OnlineCk");

    rbuflogd_producer_log(
        &loggerProducer, 
        RBUF_LOG_LEVEL_INFO, 
        "OnlineChecker", 
        "Initialized OnlineChecker with CURL");
}

OnlineChecker::~OnlineChecker() {
    curl_global_cleanup();
    rbuflogd_producer_log(
        &loggerProducer, 
        RBUF_LOG_LEVEL_INFO, 
        "OnlineChecker", 
        "Cleaned up CURL resources");

    rbuflogd_producer_close(&loggerProducer);
}

bool OnlineChecker::isOnline() const {
    std::lock_guard<std::mutex> lock(statusMutex);
    return onlineStatus;
}

uint64_t OnlineChecker::getLastResponseTimeMs() const {
    std::lock_guard<std::mutex> lock(statusMutex);
    return lastResponseTimeMs;
}

void OnlineChecker::cyclicCheck() {
    CURL* curl = curl_easy_init();
    if (!curl) {
        errorCode = 1; // Initialization failed
        setOnlineStatus(false); 
        rbuflogd_producer_log(
            &loggerProducer, 
            RBUF_LOG_LEVEL_ERROR, 
            "OnlineChecker", 
            "Failed to initialize CURL");
        return;
    }

    // ping google: 

    curl_easy_setopt(curl, CURLOPT_URL, 
        "https://clients3.google.com/generate_204");
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    BkkElapsedTimer timer;
    CURLcode result = curl_easy_perform(curl);
    long responseCode = 0;
    const uint64_t totalTimeMs = timer.elapsedMs();

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

    {
        std::lock_guard<std::mutex> lock(statusMutex);
        lastResponseTimeMs = totalTimeMs;
    }

    curl_easy_cleanup(curl);

    setOnlineStatus(
        (result == CURLE_OK && responseCode == 204) ? true : false); 

    if (isOnline()) {
        rbuflogd_producer_log(
            &loggerProducer, 
            RBUF_LOG_LEVEL_INFO, 
            "OnlineChecker", 
            QString("Online check successful in %1 ms").arg(totalTimeMs).toStdString().c_str());
    } else {
        rbuflogd_producer_log(
            &loggerProducer, 
            RBUF_LOG_LEVEL_WARNING, 
            "OnlineChecker",
            QString("Online check failed (curl=%1, http=%2, total=%3 ms)")
                .arg(static_cast<int>(result))
                .arg(responseCode)
                .arg(totalTimeMs).toStdString().c_str());
    }

    return; // Success
}
