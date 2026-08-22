#ifndef HELPER_SCREEN_REQ_HANDLER_HPP
#define HELPER_SCREEN_REQ_HANDLER_HPP

#include <QLabel>
#include "component_req_handler.hpp"

class HelperScreenReqHdl : public ComponentReqHdl {
public:
  explicit HelperScreenReqHdl(
    QWidget * parent = nullptr
  );

  virtual bkk_screen_error_code_t update_component(
    bkk_screen_uds_message_t * request,
    bkk_screen_uds_message_t * response
  ) override;

private:
  void init_ui() override;
  void refresh_ui() override;
  void set_QR_code(QLabel * image_label, const std::string & qr_code_data);

  QLabel * titleLabel = nullptr;
  QLabel * leftImageLabel = nullptr;
  QLabel * leftTextLabel = nullptr;
  QLabel * rightImageLabel = nullptr;
  QLabel * rightTextLabel = nullptr;

  helper_screen_data_t config_data {};

  const int img_xy_size_px = 200;
};

#endif // HELPER_SCREEN_REQ_HANDLER_HPP