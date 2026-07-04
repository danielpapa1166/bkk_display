#include "table_req_handler.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QMetaObject>
#include <QThread>
#include <QHeaderView>
#include <rbuflogd/logger.h>

TableReqHdl::TableReqHdl(QWidget *parent)
  : ComponentReqHdl(parent) {

  widget = new QWidget(parent);
  arrivalsTable = new QTableWidget(widget);
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



void TableReqHdl::populateTable() {

  arrivalsTable->clearContents();
  arrivalsTable->clearSpans();
  arrivalsTable->setRowCount(7);

  for (int row = 0; row < 7; row++) {
    const QColor backgroundColor 
      = (row % 2 == 0) ? QColor("#340a41") : QColor("#505050");


    // line number: 
    auto *lineItem = new QTableWidgetItem(
        QString::number(row + 1));
    lineItem->setTextAlignment(Qt::AlignCenter);
    lineItem->setBackground(backgroundColor);
    lineItem->setForeground(Qt::white);
    arrivalsTable->setItem(row, 1, lineItem);
  }

  arrivalsTable->resizeRowsToContents();
}
