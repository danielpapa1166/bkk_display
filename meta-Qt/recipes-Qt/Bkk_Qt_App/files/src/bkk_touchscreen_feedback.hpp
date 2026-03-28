#ifndef BKK_TOUCHSCREEN_FEEDBACK_HPP
#define BKK_TOUCHSCREEN_FEEDBACK_HPP

#include <QLabel>
#include <QTimer>

class TouchScreenFeedBack : public QObject {
    Q_OBJECT
public:
    explicit TouchScreenFeedBack(QLabel * dotLabel); 

    void showTouchAt(int x_raw, int y_raw);

private:
    void hideTouch();


    int x;
    int y;
    QLabel * dotLabel;
    QTimer touchDotTimer;

    const int dotRadius = 5;
};

#endif // BKK_TOUCHSCREEN_FEEDBACK_HPP