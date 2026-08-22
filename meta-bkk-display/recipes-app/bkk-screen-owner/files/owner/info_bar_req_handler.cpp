#include "info_bar_req_handler.hpp"
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QMetaObject>
#include <QThread>
#include <QDebug>
#include <QObject>
#include <QWidget>
#include <QString>
#include <rbuflogd/logger.h>

InfoBarReqHdl::InfoBarReqHdl(QWidget *parent)
    : ComponentReqHdl(parent) {
  component_id = BKK_SCREEN_COMPONENT_INFO_BAR;
  CATEGORY = "InfoBar";
  taken = false;
  key = 42;

  qt_thread_init_ui();
}


// Do not call this function directly from a non-Qt thread. 
// Use qt_thread_init_ui() instead.
void InfoBarReqHdl::init_ui() {

  if(!is_Qt_thread()) {
    log_warning(
      CATEGORY, 
      "init_ui() called from non-Qt thread"
    ); 
    return;
  }

  if(parent_widget == nullptr) {
    log_error(CATEGORY, "Parent widget is null, cannot initialize UI");
    return;
  }

  if(widget != nullptr) {
    log_warning(CATEGORY, "UI widget is already initialized");
    return;
  }

  widget = new QWidget(parent_widget);
  widget->setFixedHeight(46);

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

  ipAddressLabel = new QLabel("not yet set", widget);
  ipAddressLabel->setAlignment(Qt::AlignCenter);
  ipAddressLabel->setFixedWidth(200);
  statusRowLayout->addWidget(ipAddressLabel);

  statusRowLayout->addStretch(1);

  wifiIconLabel = new QLabel(widget);
  wifiIconLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  wifiIconLabel->setFixedSize(40, 40);
  statusRowLayout->addWidget(wifiIconLabel);

  statusRowLayout->addSpacing(12);

  statusRowLayout->addWidget(clockLabel);

  const QPixmap logo(":/icons/bkk_logo.png");
  if (!logo.isNull()) {
    bkkLogoLabel->setPixmap(
      logo.scaled(106, 40, 
        Qt::KeepAspectRatio, 
        Qt::SmoothTransformation)
    );
  } 
  else {
    log_error(CATEGORY, "Failed to load logo pixmap from Qt resource");
  }


  log_info(CATEGORY, "InfoBar UI setup complete");

  state_machine_transition(ComponentState::Ready);
}


// Do not call this function directly from a non-Qt thread. 
// Use qt_thread_refresh_ui() instead.
void InfoBarReqHdl::refresh_ui() {
  if (clockLabel == nullptr 
    || bkkLogoLabel == nullptr 
    || wifiIconLabel == nullptr
    || ipAddressLabel == nullptr) {

    log_error(CATEGORY, "UI labels are not initialized");
    return;
  }

  if(!is_Qt_thread()) {
    log_warning(
      CATEGORY, 
      "refresh_ui() called from non-Qt thread"
    ); 
    return;
  }


  clockLabel->setText(config_data.clock);
  ipAddressLabel->setText(config_data.ip_address);

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
}


bkk_screen_error_code_t InfoBarReqHdl::update_component(
    bkk_screen_uds_message_t * request,
    bkk_screen_uds_message_t * response
) {
  if(request == nullptr || response == nullptr) {
    log_error(CATEGORY, "Invalid parameters: request or response is null");
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

  memset(&config_data, 0, sizeof(config_data));

  config_data.key = request->set_info_bar_data.key;
  config_data.online_status = request->set_info_bar_data.online_status;

  strncpy(
    config_data.ip_address, 
    request->set_info_bar_data.ip_address, 
    BKK_SCREEN_IP_LEN - 1);
  config_data.ip_address[BKK_SCREEN_IP_LEN - 1] = '\0'; 

  strncpy(
    config_data.clock, 
    request->set_info_bar_data.clock, 
    BKK_SCREEN_INFO_BAR_CLOCK_MAX_LEN - 1);

  config_data.clock[BKK_SCREEN_INFO_BAR_CLOCK_MAX_LEN - 1] = '\0'; // Ensure null-termination

  qt_thread_refresh_ui();

  response->header.component_id = request->header.component_id;
  response->header.cmd_id = request->header.cmd_id;

  response->generic_resp.error_code = BKK_SCREEN_ERROR_NONE;

  return BKK_SCREEN_ERROR_NONE;
}