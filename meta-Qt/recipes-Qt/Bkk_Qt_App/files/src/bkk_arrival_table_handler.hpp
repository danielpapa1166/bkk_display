#ifndef BKK_ARRIVAL_TABLE_HANDLER_HPP
#define BKK_ARRIVAL_TABLE_HANDLER_HPP

#include <QColor>
#include <QObject>
#include <QString>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>
#include <vector>
#include "bkk_api_worker.hpp"

class ArrivalTableHandler : public QObject {
  Q_OBJECT
public:
  explicit ArrivalTableHandler(QTableWidget *table);
  ~ArrivalTableHandler();

private:
  bool blinkOn;

  // UI elements: 
  QTableWidget *arrivalsTable;

  // internal helper table handler functions: 
  QWidget* createDepartureCell(
    int departsInMin, const QColor &backgroundColor) const;
    
  QColor getRowColor(int rowIndex) const;
  void showTableMessage(const QString &message);
  void populateTable();

  // BKK API worker: 
  BkkApiWorker bkkApiWorker;
  std::vector<StationArrival> currentArrivals;
  BkkApiError apiError;
  void startApiWorker();
  void stopApiWorker();

  void handleApiFetchCompleted();

  QTimer bkkWorkerUpdateTimer;


  // config constants:
  static constexpr int kMaxRows = 8;
  static constexpr int kBlinkThresholdGreen = 10; 
  static constexpr int kBlinkThresholdRed = 5; 

  static constexpr int kBlinkIntervalMs = 800; // blink interval 
  static constexpr int kFetchIntervalMs = 5000; // fetch new data 

};

#endif // BKK_ARRIVAL_TABLE_HANDLER_HPP
