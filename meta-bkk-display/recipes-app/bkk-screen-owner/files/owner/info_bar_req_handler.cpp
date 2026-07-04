#include "info_bar_req_handler.hpp"
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QMetaObject>
#include <QThread>
#include <QDebug>
#include <QObject>
#include <QWidget>
#include <rbuflogd/logger.h>

InfoBarReqHdl::InfoBarReqHdl(QWidget *parent)
    : ComponentReqHdl(parent) {
  component_id = BKK_SCREEN_COMPONENT_INFO_BAR;
  widget = new QWidget(parent);
  taken = false;
  key = 42;

  widget->setFixedHeight(46);
  setup_ui();
}

void InfoBarReqHdl::setup_ui() {
  auto *statusRowLayout = new QHBoxLayout(widget);
  statusRowLayout->setContentsMargins(0, 0, 0, 0);
  statusRowLayout->setSpacing(8);

  clockLabel = new QLabel(widget);
  clockLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  clockLabel->setFixedWidth(86);

  bkkLogoLabel = new QLabel(widget);
  bkkLogoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  bkkLogoLabel->setFixedSize(106, 40);
  statusRowLayout->addWidget(bkkLogoLabel);

  statusRowLayout->addStretch(1);

  wifiIconLabel = new QLabel(widget);
  wifiIconLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  wifiIconLabel->setFixedSize(40, 40);
  statusRowLayout->addWidget(wifiIconLabel);

  statusRowLayout->addSpacing(12);

  statusRowLayout->addWidget(clockLabel);

  log_info(CATEGORY, "InfoBar UI setup complete");
}


bkk_screen_error_code_t InfoBarReqHdl::update_component(
    bkk_screen_uds_message_t * request,
    bkk_screen_uds_message_t * response
) {
  if(request == nullptr || response == nullptr) {
    return BKK_SCREEN_ERROR_INVALID_PARAM;
  }


  bkk_screen_set_info_bar_data_t config_data {};
  config_data.key = request->set_info_bar_data.key;
  config_data.online_status = request->set_info_bar_data.online_status;
  
  strncpy(
    config_data.clock, 
    request->set_info_bar_data.clock, 
    BKK_SCREEN_INFO_BAR_CLOCK_MAX_LEN - 1);

  config_data.clock[BKK_SCREEN_INFO_BAR_CLOCK_MAX_LEN - 1] = '\0'; // Ensure null-termination


  auto apply_ui = [this, config_data]() {
    if (clockLabel == nullptr || bkkLogoLabel == nullptr || wifiIconLabel == nullptr) {
      log_error(CATEGORY, "UI labels are not initialized");
      return;
    }

    const QPixmap logo(":/icons/bkk_logo.png");
    if (!logo.isNull()) {
      bkkLogoLabel->setPixmap(
        logo.scaled(106, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation)
      );
    } 
    else {
      log_error(CATEGORY, "Failed to load logo pixmap from Qt resource");
    }

    clockLabel->setText(config_data.clock);

    const char *wifi_icon_path =
      config_data.online_status == BKK_SCREEN_ONLINE_STATUS_ONLINE
        ? ":/icons/wifi_on.png"
        : ":/icons/wifi_off.png";

    const QPixmap wifi_icon(wifi_icon_path);
    if (!wifi_icon.isNull()) {
      wifiIconLabel->setPixmap(
        wifi_icon.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation)
      );
    } 
    else {
      log_error(CATEGORY, 
        ("Failed to load Wi-Fi icon pixmap from Qt resource: " 
          + std::string(wifi_icon_path)).c_str());
    }
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