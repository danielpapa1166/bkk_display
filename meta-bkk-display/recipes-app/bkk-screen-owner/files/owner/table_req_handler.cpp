#include "table_req_handler.hpp"
#include "bkk_screen_client/common_defs.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QMetaObject>
#include <QThread>
#include <QHeaderView>
#include <QSizePolicy>
#include <rbuflogd/logger.h>

TableReqHdl::TableReqHdl(QWidget *parent)
  : ComponentReqHdl(parent) {

  widget = new QWidget(parent);
  arrivalsTable = new QTableWidget(widget);
  auto *tableLayout = new QVBoxLayout(widget);
  tableLayout->setContentsMargins(0, 0, 0, 0);
  tableLayout->setSpacing(0);
  tableLayout->addWidget(arrivalsTable);

  widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  arrivalsTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  component_id = BKK_SCREEN_COMPONENT_TABLE;
  taken = false;
  key = 43;


  setup_ui();
}


void TableReqHdl::setup_ui() {
  // Implement the UI setup here

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

}


bkk_screen_error_code_t TableReqHdl::update_component(
  bkk_screen_uds_message_t * request,
  bkk_screen_uds_message_t * response
) {
  // Implement the update logic here


  auto apply_ui = [this]() {
    if (arrivalsTable == nullptr) {
      log_error(CATEGORY, "UI widget is not initialized");
      return;
    }

    populateTable();

  };

  if (QThread::currentThread() == thread()) {
    apply_ui();
  } 
  else {
    QMetaObject::invokeMethod(this, apply_ui, Qt::QueuedConnection);
  }

  response->header.component_id = request->header.component_id;
  response->header.cmd_id = request->header.cmd_id;
  response->generic_resp.error_code = BKK_SCREEN_ERROR_NONE;
  return BKK_SCREEN_ERROR_NONE;
}


QWidget *TableReqHdl::createDepartureCell(
  int departsInMin, const QColor &backgroundColor) const {
  auto *container = new QWidget(arrivalsTable);
  container->setStyleSheet(QString("background-color: %1;").arg(backgroundColor.name()));

  auto *layout = new QHBoxLayout(container);
  layout->setContentsMargins(6, 0, 6, 0);
  layout->setSpacing(6);
  layout->setAlignment(Qt::AlignCenter);

  auto *dot = new QLabel(container);
  dot->setFixedSize(8, 8);

  bool blinkOn = true; 
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


void TableReqHdl::populateTable() {

  arrivalsTable->clearContents();
  arrivalsTable->clearSpans();
  arrivalsTable->setRowCount(7);

  static const arrival_info_t arrivals[7] = {
    {0, "Station A", "1", "Destination X", 1},
    {0, "Station B", "2", "Destination Y", 5},
    {0, "Station C", "3", "Destination Z", 10},
    {0, "Station D", "4", "Destination W", 15},
    {0, "Station E", "5", "Destination V", 20},
    {0, "Station F", "6", "Destination U", 25},
    {0, "Station G", "7", "Destination T", 30}
  };

  for (int row = 0; row < sizeof(arrivals) / sizeof(arrivals[0]); row++) {
    const QColor backgroundColor 
      = (row % 2 == 0) ? QColor("#340a41") : QColor("#505050");

   const auto &stationArrival = arrivals[static_cast<size_t>(row)];

    // stop name (truncated to x characters):
    auto *stopItem = new QTableWidgetItem(
        QString::fromStdString(stationArrival.station).left(16));
    stopItem->setTextAlignment(Qt::AlignCenter);
    stopItem->setBackground(backgroundColor);
    stopItem->setForeground(Qt::white);
    arrivalsTable->setItem(row, 0, stopItem);

    // line number: 
    auto *lineItem = new QTableWidgetItem(
        QString::fromStdString(stationArrival.line));
    lineItem->setTextAlignment(Qt::AlignCenter);
    lineItem->setBackground(backgroundColor);
    lineItem->setForeground(Qt::white);
    arrivalsTable->setItem(row, 1, lineItem);

    // destination:
    auto *destinationItem = new QTableWidgetItem(
        QString::fromStdString(stationArrival.destination).left(16));
    destinationItem->setBackground(backgroundColor);
    destinationItem->setForeground(Qt::white);
    arrivalsTable->setItem(row, 2, destinationItem);

    // departure time:
    arrivalsTable->setCellWidget(row, 3,
      createDepartureCell(stationArrival.departure_time, backgroundColor));

  }

  arrivalsTable->resizeRowsToContents();
}
