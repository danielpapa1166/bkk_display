#ifndef INFO_BAR_REQ_HANDLER_HPP
#define INFO_BAR_REQ_HANDLER_HPP

#include <QWidget>
#include <QObject>
#include <QLabel>
#include "bkk_screen_common_priv_defs.hpp"
#include "component_req_handler.hpp"

class InfoBarReqHdl : public ComponentReqHdl {
  Q_OBJECT
public:
  explicit InfoBarReqHdl(QWidget *parent = nullptr);

  bkk_screen_error_code_t update_component(
    bkk_screen_uds_message_t * request,
    bkk_screen_uds_message_t * response
  ) override;

private: 
  QLabel * clockLabel = nullptr;
  QLabel * bkkLogoLabel = nullptr;
  QLabel * wifiIconLabel = nullptr;

  bkk_screen_set_info_bar_data_t config_data {};

  void init_ui() override;
  void refresh_ui() override;

};


#endif // INFO_BAR_REQ_HANDLER_HPP