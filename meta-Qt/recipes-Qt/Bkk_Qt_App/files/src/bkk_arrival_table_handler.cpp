#include "bkk_arrival_table_handler.hpp"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidgetItem>
#include <algorithm>

ArrivalTableHandler::ArrivalTableHandler(QTableWidget *table) 
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

    // stop name: 
    auto *stopItem = new QTableWidgetItem(
        QString::fromStdString(stationArrival.station_name));
    stopItem->setTextAlignment(Qt::AlignCenter);
    stopItem->setBackground(backgroundColor);
    stopItem->setForeground(Qt::white);
    arrivalsTable->setItem(row, 0, stopItem);

    // line number: 
    auto *lineItem = new QTableWidgetItem(
        QString::fromStdString(stationArrival.arrival.line));
    lineItem->setTextAlignment(Qt::AlignCenter);
    lineItem->setBackground(backgroundColor);
    lineItem->setForeground(Qt::white);
    arrivalsTable->setItem(row, 1, lineItem);

    // destination:
    auto *destinationItem = new QTableWidgetItem(
        QString::fromStdString(stationArrival.arrival.destination));
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
