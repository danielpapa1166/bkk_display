#include <unistd.h>
#include "screen.hpp"



BkkScreen::BkkScreen(QWidget *parent)
    : QWidget(parent)
{
  setup_base_ui();
}

void BkkScreen::setup_base_ui() {
  setMinimumSize(480, 320);
  setWindowTitle("BKK Display");

  setStyleSheet(
    "QWidget { background-color: #340a41; color: #ffffff; }"
    "QLabel  { background-color: #340a41; color: #ffffff; }"
    "QHeaderView::section { background-color: #505050; color: #ffffff; "
    "                        border: none; padding: 4px; font-weight: bold; }"
    "QTableWidget { background-color: #340a41; gridline-color: #505050;"
    "                border: none; }"
    "QTableCornerButton::section { background-color: #505050; }"
  );

  layout = new QVBoxLayout(this);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(6);

  infoBar = new QWidget(this);
  infoBar->setFixedHeight(46);
  layout->addWidget(infoBar);

  contentWidget = new QWidget(this);
  layout->addWidget(contentWidget, 1);
}
