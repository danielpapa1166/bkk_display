#include "bkk_arrival_table_handler.hpp"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidgetItem>
#include <algorithm>

ArrivalTableHandler::ArrivalTableHandler(QTableWidget *table, const char *apiKeyPath) 
    : QObject(table), arrivalsTable(table), 
    apiError(BkkApiError::None), blinkOn(false) {
  
  // setup GUI table widget:
  arrivalsTable->setColumnCount(4);
  arrivalsTable->setHorizontalHeaderLabels(
    {"Station", "Line", "Destination", "Departs"});

  arrivalsTable->horizontalHeader()
    ->setStretchLastSection(false);
  arrivalsTable->horizontalHeader()
    ->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  arrivalsTable->horizontalHeader()
    ->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  arrivalsTable->horizontalHeader()
    ->setSectionResizeMode(2, QHeaderView::Stretch);
  arrivalsTable->horizontalHeader()
    ->setSectionResizeMode(3, QHeaderView::ResizeToContents);

  arrivalsTable->verticalHeader()->setVisible(false);
  arrivalsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  arrivalsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  arrivalsTable->setSelectionMode(QAbstractItemView::SingleSelection);
  arrivalsTable->setAlternatingRowColors(false);
  arrivalsTable->setSortingEnabled(false);
  
  int res = bkkApiWorker.loadApiKey(apiKeyPath);
  if(res != 0) {
    showTableMessage("Error loading API key");
    return;
  }
  startApiWorker();
  // start a timer to periodically update the table: 
  QObject::connect(&bkkWorkerUpdateTimer, &QTimer::timeout, this, 
    [this]() {
      bkkApiWorker.requestFetch();
    }
  );
  bkkWorkerUpdateTimer.start(kFetchIntervalMs); 

  // start blink timer for departure dots:
  QTimer *blinkTimer = new QTimer(this);
  QObject::connect(blinkTimer, &QTimer::timeout, this, [this]() {
    blinkOn = !blinkOn;
    populateTable();
  });
  blinkTimer->start(kBlinkIntervalMs); // toggle blink every second
}


ArrivalTableHandler::~ArrivalTableHandler() {
  stopApiWorker();
}


QWidget *ArrivalTableHandler::createLineIdCell(const Arrival & arrival, 
    const QColor &backgroundColor) const {

  (void) backgroundColor;

  auto *container = new QWidget(arrivalsTable);
  container->setStyleSheet(QString("background-color: %1;").arg(backgroundColor.name()));
  auto *layout = new QHBoxLayout(container);
  layout->setContentsMargins(6, 0, 6, 0);
  layout->setSpacing(6);
  layout->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);

  QString lineBoxColor;

  switch (arrival.vehicle_type) {
    case VEHICLE_TYPE_BUS:
      lineBoxColor = "#009ee3"; 
      break;
    case VEHICLE_TYPE_TRAM:
      lineBoxColor = "#ffd900"; 
      break;
    case VEHICLE_TYPE_METRO:
      lineBoxColor = "#F9AB13"; 
      break;
    default:
      lineBoxColor = "#1e1e1e"; 
      break;
  }


  auto *lineBox = new QWidget(container);
  //lineBox->setFixedSize(60, 60);
  lineBox->setMinimumSize(60, 10);
  lineBox->setStyleSheet(QString(
      "background-color: %1; border-radius: 8px; border: none;").arg(lineBoxColor));

  auto *lineBoxLayout = new QHBoxLayout(lineBox);
  lineBoxLayout->setContentsMargins(10, 3, 10, 3);
  lineBoxLayout->setSpacing(0);

  auto *lineLabel = new QLabel(QString::fromStdString(arrival.line_id), lineBox);
  lineLabel->setAlignment(Qt::AlignCenter);
  lineLabel->setStyleSheet("color: #ffffff; background: transparent;");

  lineBoxLayout->addWidget(lineLabel);
  layout->addWidget(lineBox); 

  return container;

}


QWidget *ArrivalTableHandler::createDepartureCell(
  int departsInMin, const QColor &backgroundColor) const {
  auto *container = new QWidget(arrivalsTable);
  container->setStyleSheet(QString("background-color: %1;").arg(backgroundColor.name()));

  auto *layout = new QHBoxLayout(container);
  layout->setContentsMargins(6, 0, 6, 0);
  layout->setSpacing(6);
  layout->setAlignment(Qt::AlignCenter);

  auto *dot = new QLabel(container);
  dot->setFixedSize(8, 8);

  QString dotColor = "transparent";
  if (blinkOn) {
    if (departsInMin < kBlinkThresholdRed) {
      dotColor = "#ff2d2d";
    } 
    else if (departsInMin <= kBlinkThresholdGreen) {
      dotColor = "#00d84f";
    }
  }
  dot->setStyleSheet(QString("background-color: %1; border-radius: 4px;")
    .arg(dotColor));

  auto *minutes = new QLabel(QString::number(departsInMin) + "'", container);
  minutes->setAlignment(Qt::AlignCenter);
  minutes->setStyleSheet("color: #ffffff;");

  layout->addWidget(dot);
  layout->addWidget(minutes);
  return container;
}

QColor ArrivalTableHandler::getRowColor(int rowIndex) const {
  return (rowIndex % 2 == 0) 
    ? QColor("#340a41") 
    : QColor("#505050");
}

void ArrivalTableHandler::showTableMessage(const QString &message)
{
  arrivalsTable->clearContents();
  arrivalsTable->clearSpans();
  arrivalsTable->setRowCount(1);

  auto *messageItem = new QTableWidgetItem(message);
  messageItem->setTextAlignment(Qt::AlignCenter);
  arrivalsTable->setSpan(0, 0, 1, arrivalsTable->columnCount());
  arrivalsTable->setItem(0, 0, messageItem);
}


void ArrivalTableHandler::populateTable() {

  std::sort(currentArrivals.begin(), currentArrivals.end(), 
      [](const StationArrival &left, const StationArrival &right) {
    // sort by departure time
    return left.arrival.departs_in_min < right.arrival.departs_in_min;
  });

  // limit to max rows:
  if (static_cast<int>(currentArrivals.size()) > kMaxRows) {
    currentArrivals.resize(static_cast<size_t>(kMaxRows));
  }

  if (apiError != BkkApiError::None) {
    showTableMessage("Error fetching arrivals");
    return;
  }

  if (currentArrivals.empty()) {
    showTableMessage("No arrivals");
    return;
  }

  arrivalsTable->clearContents();
  arrivalsTable->clearSpans();
  arrivalsTable->setRowCount(static_cast<int>(currentArrivals.size()));

  for (int row = 0; row < static_cast<int>(currentArrivals.size()); row++) {
    const auto &stationArrival = currentArrivals[static_cast<size_t>(row)];
    const QColor backgroundColor = getRowColor(row);

    // stop name (truncated to x characters):
    auto *stopItem = new QTableWidgetItem(
        QString::fromStdString(stationArrival.station_name).left(16));
    stopItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    stopItem->setBackground(backgroundColor);
    stopItem->setForeground(Qt::white);
    arrivalsTable->setItem(row, 0, stopItem);

    // line number: 
    arrivalsTable->setCellWidget(row, 1,
      createLineIdCell(stationArrival.arrival, backgroundColor));

    // destination:
    auto *destinationItem = new QTableWidgetItem(
        QString::fromStdString(stationArrival.arrival.destination).left(20));
    destinationItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    destinationItem->setBackground(backgroundColor);
    destinationItem->setForeground(Qt::white);
    arrivalsTable->setItem(row, 2, destinationItem);

    // departure time:
    arrivalsTable->setCellWidget(row, 3,
      createDepartureCell(stationArrival.arrival.departs_in_min, backgroundColor));

  }

  arrivalsTable->resizeRowsToContents();
}


void ArrivalTableHandler::startApiWorker() {
  // connect signal callback function: 
  QObject::connect(&bkkApiWorker, &BkkApiWorker::fetchCompleted, this, 
    [this]() {
      handleApiFetchCompleted();
    }
  );
  bkkApiWorker.start();
}

void ArrivalTableHandler::stopApiWorker() {
  bkkApiWorker.requestInterruption();
  bkkApiWorker.wait();
}


void ArrivalTableHandler::handleApiFetchCompleted() {
  // called in worker thread: store response later to be displayed: 
  currentArrivals = bkkApiWorker.getArrivals();
  apiError = bkkApiWorker.getErrorCode();
}
