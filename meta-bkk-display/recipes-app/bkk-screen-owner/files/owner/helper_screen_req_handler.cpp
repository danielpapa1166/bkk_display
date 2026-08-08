#include "helper_screen_req_handler.hpp"
#include "bkk_screen_client/common_defs.hpp"
#include <QHBoxLayout>
#include <QObject>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>
#include <cstring>
#include <QString>
#include <rbuflogd/logger.h>


HelperScreenReqHdl::HelperScreenReqHdl(QWidget * parent)
  : ComponentReqHdl(parent) {

  CATEGORY = "HelpScr";
  log_info(CATEGORY, "Initializing HelperScreenReqHdl");
  component_id = BKK_SCREEN_COMPONENT_HELPER_SCREEN;

  taken = false;
  key = 44;

  qt_thread_init_ui();

  log_info(CATEGORY, "HelperScreenReqHdl initialized");
}


void HelperScreenReqHdl::init_ui() {
  if (!is_Qt_thread()) {
    log_warning(CATEGORY, "init_ui() called from non-Qt thread");
    return;
  }

  if (parent_widget == nullptr || widget != nullptr) {
    return;
  }

  widget = new QWidget(parent_widget);

  auto * mainLayout = new QVBoxLayout(widget);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setAlignment(Qt::AlignCenter);

  mainLayout->addStretch();

  titleLabel = new QLabel("Helper Screen", widget);
  titleLabel->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(titleLabel, 0, Qt::AlignHCenter);

  auto * columnsLayout = new QHBoxLayout();
  columnsLayout->setAlignment(Qt::AlignCenter);

  auto * leftColumnLayout = new QVBoxLayout();
  leftImageLabel = new QLabel(widget);
  leftImageLabel->setFixedSize(img_xy_size_px, img_xy_size_px);
  leftImageLabel->setAlignment(Qt::AlignCenter);
  leftTextLabel = new QLabel("", widget);
  leftTextLabel->setAlignment(Qt::AlignCenter);
  leftColumnLayout->addWidget(leftImageLabel, 1);
  leftColumnLayout->addWidget(leftTextLabel);

  auto * rightColumnLayout = new QVBoxLayout();
  rightImageLabel = new QLabel(widget);
  rightImageLabel->setFixedSize(img_xy_size_px, img_xy_size_px);
  rightImageLabel->setAlignment(Qt::AlignCenter);
  rightTextLabel = new QLabel("", widget);
  rightTextLabel->setAlignment(Qt::AlignCenter);
  rightColumnLayout->addWidget(rightImageLabel, 1);
  rightColumnLayout->addWidget(rightTextLabel);

  columnsLayout->addLayout(leftColumnLayout);
  columnsLayout->addLayout(rightColumnLayout);
  mainLayout->addLayout(columnsLayout);
  mainLayout->addStretch();

  state_machine_transition(ComponentState::Ready);
}


void HelperScreenReqHdl::refresh_ui() {
  if (!is_Qt_thread()) {
    log_warning(CATEGORY, "refresh_ui() called from non-Qt thread");
    return;
  }

  if (titleLabel == nullptr || leftImageLabel == nullptr
    || leftTextLabel == nullptr || rightImageLabel == nullptr
    || rightTextLabel == nullptr) {
    log_error(CATEGORY, "UI labels are not initialized");
    return;
  }

  titleLabel->setText(config_data.helper_title);
  leftTextLabel->setText(config_data.helper_text[0]);
  rightTextLabel->setText(config_data.helper_text[1]);

  set_image(leftImageLabel, config_data.helper_image_id[0]);
  set_image(rightImageLabel, config_data.helper_image_id[1]);
}


void HelperScreenReqHdl::set_image(QLabel * image_label,
    helper_screen_img_t image_id) {
  const int image_index = static_cast<int>(image_id);
  if (image_index < BKK_SCREEN_HELPER_IMG_TEST_0
    || image_index > BKK_SCREEN_HELPER_IMG_TEST_3) {
    image_label->clear();
    log_warning(CATEGORY, "Invalid helper screen image ID");
    return;
  }

  // to be replaced by actual image resource paths 
  char image_path[64];
  snprintf(image_path, 
    sizeof(image_path), 
    ":/icons/image_%d.png", 
    image_index);

  const QPixmap image(image_path);

  if (image.isNull()) {
    image_label->clear();
    log_error(CATEGORY, "Failed to load helper screen image resource");
    return;
  }

  image_label->setPixmap(image.scaled(
    img_xy_size_px,
    img_xy_size_px,
    Qt::KeepAspectRatio,
    Qt::SmoothTransformation));
}

bkk_screen_error_code_t HelperScreenReqHdl::update_component(
    bkk_screen_uds_message_t * request,
    bkk_screen_uds_message_t * response
) {
  if(request == nullptr || response == nullptr) {
    return BKK_SCREEN_ERROR_INVALID_PARAM;
  }

  if (state != ComponentState::Acquired) {
    response->header.component_id = request->header.component_id;
    response->header.cmd_id = request->header.cmd_id;
    response->generic_resp.error_code = BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND;
    log_warning(CATEGORY,
      ("Update request received for component "
        + get_component_name()
        + " which is not taken").c_str());
    return BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND;
  }

  if (request->header.cmd_id == BKK_SCREEN_COMMAND_SET_DATA) {
    config_data = request->set_helper_screen_data.helper_data;
    config_data.helper_title[BKK_SCREEN_HELPER_TITLE_MAX_LEN - 1] = '\0';
    for (int columnIndex = 0;
      columnIndex < BKK_SCREEN_HELPER_MAX_NUM_OF_COLS;
      columnIndex++) {
      config_data.helper_text[columnIndex][BKK_SCREEN_HELPER_TEXT_MAX_LEN - 1]
        = '\0';
    }
  }

  qt_thread_refresh_ui();

  log_info(CATEGORY, "Updating helper screen component");

  response->header.component_id = request->header.component_id;
  response->header.cmd_id = request->header.cmd_id;
  response->generic_resp.error_code = BKK_SCREEN_ERROR_NONE;

  return BKK_SCREEN_ERROR_NONE;
}
