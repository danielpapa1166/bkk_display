#include "info_bar_req_handler.hpp"
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QMetaObject>
#include <QThread>
#include <QDebug>
#include <QObject>
#include <QWidget>

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
}


int InfoBarReqHdl::update_component(
    void * request,
    bkk_screen_uds_response_t * response
) {
  if(request == nullptr || response == nullptr) {
    return static_cast<int>(BKK_SCREEN_INTERNAL_UDS_ERR_INVALID_PARAM);
  }

  bkk_screen_info_bar_data_t * info_bar_data = 
    static_cast<bkk_screen_info_bar_data_t *>(request);

  bkk_screen_info_bar_data_t data_copy = *info_bar_data;

  // for now just print the data to console:
  printf("Info Bar Data Received:\n");
  printf("Clock: %s\n", info_bar_data->clock);
  printf("Online Status: %s\n", 
    info_bar_data->online_status 
    == BKK_SCREEN_ONLINE_STATUS_ONLINE ? "Online" : "Offline"
  );


  auto apply_ui = [this, data_copy]() {
    if (clockLabel == nullptr || bkkLogoLabel == nullptr || wifiIconLabel == nullptr) {
      printf("Info bar labels are not initialized\n");
      return;
    }

    const QPixmap logo(":/icons/bkk_logo.png");
    if (!logo.isNull()) {
      bkkLogoLabel->setPixmap(
        logo.scaled(106, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation)
      );
    } else {
      printf("Failed to load logo pixmap from Qt resource\n");
    }

    clockLabel->setText(data_copy.clock);

    const char *wifi_icon_path =
      data_copy.online_status == BKK_SCREEN_ONLINE_STATUS_ONLINE
        ? ":/icons/wifi_on.png"
        : ":/icons/wifi_off.png";

    const QPixmap wifi_icon(wifi_icon_path);
    if (!wifi_icon.isNull()) {
      wifiIconLabel->setPixmap(
        wifi_icon.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation)
      );
    } 
    else {
      printf("Failed to load Wi-Fi icon pixmap"
        " from Qt resource: %s\n", wifi_icon_path);
    }
  };

  if (QThread::currentThread() == thread()) {
    apply_ui();
  } 
  else {
    QMetaObject::invokeMethod(this, apply_ui, Qt::QueuedConnection);
  }

  //response->error_code = BKK_SCREEN_ERROR_NONE;
  return static_cast<int>(BKK_SCREEN_INTERNAL_UDS_ERR_NONE);
}