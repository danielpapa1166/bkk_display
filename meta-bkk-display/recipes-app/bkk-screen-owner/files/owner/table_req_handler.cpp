#include "table_req_handler.hpp"
#include "bkk_screen_client/common_defs.hpp"
#include <bkk_utils/bkk_utils_timing.h>
#include <QVBoxLayout>
#include <QLabel>
#include <QMetaObject>
#include <QThread>
#include <QHeaderView>
#include <QSizePolicy>
#include <rbuflogd/logger.h>
#include <algorithm>
#include <utility>

typedef enum {
  VEHICLE_TYPE_UNKNOWN,
  VEHICLE_TYPE_BUS,
  VEHICLE_TYPE_TRAM,
  VEHICLE_TYPE_TROLLEYBUS,
  VEHICLE_TYPE_METRO,
  VEHICLE_TYPE_SUBURB_RAIL,
  VEHICLE_TYPE_RAIL,
  VEHICLE_TYPE_FERRY,
  VEHICLE_TYPE_CABLE_CAR,
  VEHICLE_TYPE_FUNICULAR,
  VEHICLE_TYPE_GONDOLA,
  VEHICLE_TYPE_COACH,
  VEHICLE_TYPE_BICYCLE,
  VEHICLE_TYPE_CAR,
  VEHICLE_TYPE_WALK,
  VEHICLE_TYPE_LOCAL,
  VEHICLE_TYPE_TRANSIT
} vehicle_screen_type_t;


int TableReqHdl::blink_screen_thread_func(void * arg) {
  TableReqHdl * self = static_cast<TableReqHdl *>(arg);
  if (self == nullptr) {
    log_error("Blink", "Invalid argument for blink_screen_thread_func");
    return -1;
  }

  self->blinkState = !self->blinkState;
  self->qt_thread_refresh_ui();
  return 0;
}

TableReqHdl::TableReqHdl(QWidget *parent)
  : ComponentReqHdl(parent) {

  component_id = BKK_SCREEN_COMPONENT_TABLE;
  CATEGORY = "Table";
  taken = false;
  key = 43;


  qt_thread_init_ui();
}


// Do not call this function directly from a non-Qt thread. 
// Use qt_thread_init_ui() instead.
void TableReqHdl::init_ui() {
  // Implement the UI setup here
  if(!is_Qt_thread()) {
    log_warning(
      CATEGORY, 
      "init_ui() called from non-Qt thread"
    ); 
    return;
  }

  widget = new QWidget(parent_widget);
  arrivalsTable = new QTableWidget(widget);
  auto *tableLayout = new QVBoxLayout(widget);
  tableLayout->setContentsMargins(0, 0, 0, 0);
  tableLayout->setSpacing(0);
  tableLayout->addWidget(arrivalsTable);

  widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  arrivalsTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);


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

  state_machine_transition(ComponentState::Ready);
}


// Do not call this function directly from a non-Qt thread. 
// Use qt_thread_refresh_ui() instead.
void TableReqHdl::refresh_ui() {

  if(!is_Qt_thread()) {
    log_warning(
      CATEGORY, 
      "refresh_ui() called from non-Qt thread"
    ); 
    return;
  }

  if (arrivalsTable == nullptr) {
    log_error(CATEGORY, "UI widget is not initialized");
    return;
  }
  populateTable();
}


bkk_screen_error_code_t TableReqHdl::update_component(
  bkk_screen_uds_message_t * request,
  bkk_screen_uds_message_t * response
) {
  if (request == nullptr || response == nullptr) {
    return BKK_SCREEN_ERROR_INVALID_PARAM;
  }

  if(state != ComponentState::Acquired) {
    response->header.component_id = request->header.component_id;
    response->header.cmd_id = request->header.cmd_id;
    response->generic_resp.error_code = BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND;
    log_warning(CATEGORY,
      ("Update request received for component "
        + get_component_name()
        + " which is not taken").c_str());
    return BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND;
  }

  if(!blink_timer_ctx.is_running) {
    bkk_setup_timer_with_callback(&blink_timer_ctx);
  }

  int row_count = request->set_table_data.num_arrivals;
  row_count = std::clamp(row_count, 0, BKK_SCREEN_MAX_ARRIVALS);

  
  arrivals.clear();
  arrivals.reserve(static_cast<size_t>(row_count));
  for (int i = 0; i < row_count; ++i) {
    arrival_info_t item = request->set_table_data.arrivals[i];

    // Ensure network payload strings are always terminated.
    item.station[BKK_SCREEN_STATION_NAME_MAX_LEN - 1] = '\0';
    item.line[BKK_SCREEN_LINE_NAME_MAX_LEN - 1] = '\0';
    item.destination[BKK_SCREEN_DESTINATION_NAME_MAX_LEN - 1] = '\0';
    arrivals.push_back(item);
  }

  qt_thread_refresh_ui();

  response->header.component_id = request->header.component_id;
  response->header.cmd_id = request->header.cmd_id;
  response->generic_resp.error_code = BKK_SCREEN_ERROR_NONE;
  return BKK_SCREEN_ERROR_NONE;
}


void TableReqHdl::qt_thread_clear_component() {
  log_info(CATEGORY, 
    ("Clearing component " + get_component_name()).c_str());
  taken = false;
  key = -1; 
  alive_counter = MAX_ALIVE_COUNTER;
  arrivals.clear();

  if(blink_timer_ctx.is_running) {
    // stop timer thread if running: 
    bkk_stop_timer_with_callback(&blink_timer_ctx);
    bkk_join_timer_with_callback(&blink_timer_ctx);
  }

  if (QThread::currentThread() == thread()) {
    if (arrivalsTable != nullptr) {
      arrivalsTable->setRowCount(0);
      arrivalsTable->clearContents();
      arrivalsTable->clearSpans();
      arrivalsTable->hide();
      arrivalsTable->deleteLater();
      arrivalsTable = nullptr;
    }

    if (widget != nullptr) {
      widget->hide();
      widget->deleteLater();
      widget = nullptr;
    }
    state_machine_transition(ComponentState::Empty);
    return;
  }
  QMetaObject::invokeMethod(this, [this]() {
    if (arrivalsTable != nullptr) {
      arrivalsTable->setRowCount(0);
      arrivalsTable->clearContents();
      arrivalsTable->clearSpans();
      arrivalsTable->hide();
      arrivalsTable->deleteLater();
      arrivalsTable = nullptr;
    }

    if (widget != nullptr) {
      widget->hide();
      widget->deleteLater();
      widget = nullptr;
    }
    state_machine_transition(ComponentState::Empty);
  }, Qt::BlockingQueuedConnection);

  log_info(CATEGORY, 
    ("Component " + get_component_name() + " cleared").c_str());
}



QWidget *TableReqHdl::createLineIdCell(const arrival_info_t & arrival,
    const QColor &backgroundColor) const {

  static const QString defaultLineBoxColor = "#1e1e1e90"; 
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
      lineBoxColor = "#e0bf00"; 
      break;
    case VEHICLE_TYPE_TROLLEYBUS: 
      lineBoxColor = "#e41f18"; 
      break;
    case VEHICLE_TYPE_METRO:

      if(strcmp(arrival.line, "M1") == 0) {
        lineBoxColor = "#e0bf00"; 
      } 
      else if(strcmp(arrival.line, "M2") == 0) {
        lineBoxColor = "#ff0000"; 
      } 
      else if(strcmp(arrival.line, "M3") == 0) {
        lineBoxColor = "#005ca5"; 
      } 
      else if(strcmp(arrival.line, "M4") == 0) {
        lineBoxColor = "#4ca22f";
      } 
      else {
        lineBoxColor = defaultLineBoxColor; // default color
      }
      break;

    default:
      lineBoxColor = defaultLineBoxColor; 
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

  auto *lineLabel = new QLabel(QString::fromStdString(arrival.line), lineBox);
  lineLabel->setAlignment(Qt::AlignCenter);
  lineLabel->setStyleSheet("color: #ffffff; background: transparent;");

  lineBoxLayout->addWidget(lineLabel);
  layout->addWidget(lineBox); 

  return container;

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

  bool blinkOn = blinkState;
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
  arrivalsTable->setRowCount(kMaxRows);

  const int rowsToRender = std::min(
    static_cast<int>(arrivals.size()),
    kMaxRows
  );

  for (int row = 0; row < rowsToRender; row++) {
    const QColor backgroundColor 
      = (row % 2 == 0) ? QColor("#340a41") : QColor("#505050");

   const auto &stationArrival = arrivals[static_cast<size_t>(row)];

    // stop name (truncated to x characters):
    auto *stopItem = new QTableWidgetItem(
        QString::fromUtf8(stationArrival.station).left(numOfStationCharsToDisplay));
    stopItem->setTextAlignment(Qt::AlignCenter);
    stopItem->setBackground(backgroundColor);
    stopItem->setForeground(Qt::white);
    arrivalsTable->setItem(row, 0, stopItem);

    // line number: 
    arrivalsTable->setCellWidget(row, 1,
      createLineIdCell(stationArrival, backgroundColor));

    // destination:
    auto *destinationItem = new QTableWidgetItem(
      QString::fromUtf8(stationArrival.destination).left(numOfDestinationCharsToDisplay));
    destinationItem->setBackground(backgroundColor);
    destinationItem->setForeground(Qt::white);
    arrivalsTable->setItem(row, 2, destinationItem);

    // departure time:
    arrivalsTable->setCellWidget(row, 3,
      createDepartureCell(stationArrival.departure_time, backgroundColor));

  }

  arrivalsTable->resizeRowsToContents();
}
