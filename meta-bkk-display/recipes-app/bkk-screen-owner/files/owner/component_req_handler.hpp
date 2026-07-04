#ifndef COMPONENT_REQ_HANDLER_HPP
#define COMPONENT_REQ_HANDLER_HPP

#include <QWidget>
#include <QObject>
#include "bkk_screen_common_priv_defs.hpp"

class ComponentReqHdl : public QObject {
  Q_OBJECT
public:
  explicit ComponentReqHdl(
    QWidget *parent = nullptr
  ) {};
  ~ComponentReqHdl() {};
  
  virtual bkk_screen_error_code_t handle_request(
    bkk_screen_uds_message_t * request, 
    bkk_screen_uds_message_t * response
  );

  virtual bkk_screen_error_code_t acquire_component(
    bkk_screen_uds_message_t * request, 
    bkk_screen_uds_message_t * response
  );

  virtual bkk_screen_error_code_t update_component(
    bkk_screen_uds_message_t * request,
    bkk_screen_uds_message_t * response
  ) = 0;

  QWidget * get_widget() const {
    return widget;
  }
  
protected: 
  bkk_screen_component_id_t component_id;
  QWidget * widget;
  bool taken;
  int key;

};

#endif // COMPONENT_REQ_HANDLER_HPP