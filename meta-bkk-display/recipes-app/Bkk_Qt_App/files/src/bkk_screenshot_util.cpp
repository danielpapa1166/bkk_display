#include "bkk_screenshot_util.hpp"

#include <rbuflogd/producer.h>

#include <QScreen>
#include <QPixmap>

void ScreenshotUtil::saveScreenshot(QScreen * screen) {
    QPixmap pixmap = screen->grabWindow(0);
    
    rbuflogd_producer_t loggerProducer {};
    rbuflogd_producer_open(&loggerProducer, "Screen");

    if(pixmap.save(save_path)) {
        rbuflogd_producer_log(
            &loggerProducer, 
            RBUF_LOG_LEVEL_DEBUG, 
            "ScreenshotUtil", 
            QString("Screenshot saved to %1").arg(save_path).toStdString().c_str());
    }
    else { 
        rbuflogd_producer_log(
            &loggerProducer, 
            RBUF_LOG_LEVEL_WARNING, 
            "ScreenshotUtil", 
            QString("Failed to save screenshot to %1").arg(save_path).toStdString().c_str());
    }

    rbuflogd_producer_close(&loggerProducer);
}