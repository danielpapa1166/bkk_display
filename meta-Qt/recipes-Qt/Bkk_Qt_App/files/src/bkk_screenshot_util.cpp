#include "bkk_screenshot_util.hpp"
#include "bkk_logger.hpp"

#include <QScreen>
#include <QPixmap>

void ScreenshotUtil::saveScreenshot(QScreen * screen) {
    QPixmap pixmap = screen->grabWindow(0);
    
    if(pixmap.save(save_path)) {
        Logger::debug("ScreenShot", "Screenshot saved to " + save_path);
    }
    else { 
        Logger::warning("ScreenShot", "Failed to save screenshot to " + save_path);
    }
}