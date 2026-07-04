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

  int update_component(
    void * request,
    bkk_screen_uds_response_t * response
  ) override;

private: 
  const char * const CATEGORY = "InfoBar";
  QLabel * clockLabel = nullptr;
  QLabel * bkkLogoLabel = nullptr;
  QLabel * wifiIconLabel = nullptr;
  void setup_ui(); 

};


#endif // INFO_BAR_REQ_HANDLER_HPP