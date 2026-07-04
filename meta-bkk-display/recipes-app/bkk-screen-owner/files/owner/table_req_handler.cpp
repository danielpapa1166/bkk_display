#include "table_req_handler.hpp"
#include <QVBoxLayout>
#include <QLabel>

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


  // set text on the base widget for testing: 
  widget->setWindowTitle("Arrivals Table");


  // setup GUI table widget:
  /*arrivalsTable->setColumnCount(4);
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
  arrivalsTable->setSortingEnabled(false);*/

}


bkk_screen_error_code_t TableReqHdl::update_component(
  bkk_screen_uds_message_t * request,
  bkk_screen_uds_message_t * response
) {
  // Implement the update logic here

  QLabel * label = new QLabel("Table component updated with new data", widget);
  QVBoxLayout * layout = new QVBoxLayout(widget);
  layout->addWidget(label);
  return BKK_SCREEN_ERROR_NONE;
}
