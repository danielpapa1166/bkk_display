#ifndef STATUS_SCREEN_REQ_HANDLER_HPP
#define STATUS_SCREEN_REQ_HANDLER_HPP
#include <QLabel>
#include "component_req_handler.hpp"

class StatusScreenReqHdl : public ComponentReqHdl {
public:
  explicit StatusScreenReqHdl(
    QWidget * parent = nullptr
  );

  virtual bkk_screen_error_code_t update_component(
    bkk_screen_uds_message_t * request,
    bkk_screen_uds_message_t * response
  ) override;

private:   
  void init_ui() override;
  void refresh_ui() override;

  bkk_screen_set_status_screen_data_t config_data {};

  QLabel * statusLabel = nullptr;
};

#endif // STATUS_SCREEN_REQ_HANDLER_HPP