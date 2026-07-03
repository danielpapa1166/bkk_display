#ifndef INFO_BAR_REQ_HANDLER_HPP
#define INFO_BAR_REQ_HANDLER_HPP

#include <QWidget>
#include <QObject>
#include "bkk_screen_common_priv_defs.hpp"
#include "component_req_handler.hpp"

class InfoBarReqHdl : public ComponentReqHdl {
  Q_OBJECT
public:
  explicit InfoBarReqHdl(QWidget *parent = nullptr);
  //~InfoBarReqHdl();

  int update_component(
    void * request,
    bkk_screen_uds_response_t * response
  ) override;

};


#endif // INFO_BAR_REQ_HANDLER_HPP