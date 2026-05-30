#include "bkk_touchscreen_feedback.hpp"


TouchScreenFeedBack::TouchScreenFeedBack(QLabel * dL) 
    : x(0), y(0), dotLabel(dL) {

    dotLabel->setFixedSize(10, 10);
    dotLabel->setStyleSheet(
        "background-color: red; border-radius: 5px;");
    dotLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    dotLabel->raise();
    dotLabel->hide();

    connect(&touchDotTimer, &QTimer::timeout, this, [this]() {
        hideTouch();
    });
}

void TouchScreenFeedBack::showTouchAt(int x, int y) {
    dotLabel->move(x - dotRadius, y - dotRadius);
    dotLabel->show();
    dotLabel->raise();
    touchDotTimer.stop();
    touchDotTimer.start(500);    
}


void TouchScreenFeedBack::hideTouch() {
    dotLabel->hide();
}