#include <fcntl.h>
#include <sys/mman.h>
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


screen_error_t BkkScreen::expose_screen_components() {
  shmem_fd = shm_open(BKK_SCREEN_SHMEM_NAME, O_RDWR | O_CREAT, 0666);
  if (shmem_fd == -1) {
    return BKK_SCREEN_ERROR_OTHER;
  }

  const int ftrunc_res = ftruncate(
    shmem_fd, 
    sizeof(bkk_screen_component_list_t));

  if (ftrunc_res == -1) {
    return BKK_SCREEN_ERROR_OTHER;
  }

  component_list = static_cast<bkk_screen_component_list_t *>(
    mmap(
      NULL, 
      sizeof(bkk_screen_component_list_t), 
      PROT_READ | PROT_WRITE, 
      MAP_SHARED, 
      shmem_fd, 
      0
    )
  );
  if (component_list == MAP_FAILED) {
    component_list = nullptr;
    return BKK_SCREEN_ERROR_OTHER;
  }


  // Expose the info bar component
  component_list->info_bar.instance = static_cast<void *>(infoBar);
  component_list->info_bar.component_id = BKK_SCREEN_COMPONENT_INFO_BAR;
  component_list->info_bar.taken = false;

  return BKK_SCREEN_ERROR_NONE;
}