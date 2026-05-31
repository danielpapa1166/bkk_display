#ifndef BKK_SCREENSHOT_UTIL_HPP
#define BKK_SCREENSHOT_UTIL_HPP

#include <QString>
#include <QScreen>

class ScreenshotUtil
{
public:
    ScreenshotUtil(QString const & save_path) : save_path(save_path) {} 
    void saveScreenshot(QScreen * screen); 

private:
    QString save_path;

};


#endif 
