#ifndef BKK_ARRIVAL_TABLE_HANDLER_HPP
#define BKK_ARRIVAL_TABLE_HANDLER_HPP


class ArrivalTableHandler : public QObject
{
    Q_OBJECT
public:
    explicit ArrivalTableHandler();
    void updateTable(const std::vector<ArrivalInfo> &arrivals, bool blinkOn);


};
