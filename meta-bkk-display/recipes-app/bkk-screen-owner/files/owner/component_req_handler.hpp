#ifndef COMPONENT_REQ_HANDLER_HPP
#define COMPONENT_REQ_HANDLER_HPP

#include <QWidget>
#include <QObject>
#include <QMetaObject>
#include <QThread>
#include "bkk_screen_common_priv_defs.hpp"
#include "qnamespace.h"
#include <rbuflogd/logger.h>

typedef enum ComponentState {
  Empty,      // UI is not initialized, component is not taken
  Ready,      // UI is initialized, component is not taken, ready for acquisition
  Acquired,   // Component is acquired, UI is initialized, component is taken
  Expiring,   // alive check expired, component is being cleaned up
  Dead        // component is dead, UI is cleaned up, component is not taken
} ComponentState_t; 

class ComponentReqHdl : public QObject {
  Q_OBJECT
public:
  // --------------------------------------------------------------------------
  // constructor and destructor
  // --------------------------------------------------------------------------
  explicit ComponentReqHdl(
    QWidget * parent = nullptr
  ) : parent_widget(parent), widget(nullptr) {};

  virtual ~ComponentReqHdl() {
    delete widget;
    widget = nullptr;
  };

  virtual void state_machine_transition(ComponentState_t new_state);
  
  // --------------------------------------------------------------------------
  // client request handlers
  // --------------------------------------------------------------------------
  virtual bkk_screen_error_code_t handle_request(
    bkk_screen_uds_message_t * request, 
    bkk_screen_uds_message_t * response
  );

  virtual bkk_screen_error_code_t acquire_component(
    bkk_screen_uds_message_t * request, 
    bkk_screen_uds_message_t * response
  );

  bkk_screen_error_code_t handle_ping_request(    
    bkk_screen_uds_message_t * request,
    bkk_screen_uds_message_t * response
  );

  bkk_screen_error_code_t handle_release_request(    
    bkk_screen_uds_message_t * request,
    bkk_screen_uds_message_t * response
  );
  
  virtual bkk_screen_error_code_t update_component(
    bkk_screen_uds_message_t * request,
    bkk_screen_uds_message_t * response
  ) = 0;

  int cyclic_alive_check();

  // --------------------------------------------------------------------------
  // UI management functions
  // --------------------------------------------------------------------------
  virtual void init_ui() = 0;
  virtual void refresh_ui() = 0;
  void qt_thread_init_ui();
  void qt_thread_refresh_ui();
  virtual void qt_thread_clear_component();

  bool is_Qt_thread() const { return QThread::currentThread() == thread(); }


  // --------------------------------------------------------------------------
  // Getters
  // --------------------------------------------------------------------------

  QWidget * get_widget() const { return widget; }
  bkk_screen_component_id_t get_component_id() const { return component_id; }
  std::string get_component_name() const;
  ComponentState_t get_state() const { return state; }
  static std::string get_state_name(ComponentState_t state);
  
protected: 
  Q_INVOKABLE void invoke_init_ui() {
    init_ui();
  }
  Q_INVOKABLE void invoke_refresh_ui() {
    refresh_ui();
  }

  ComponentState_t state = ComponentState::Empty;
  bkk_screen_component_id_t component_id;
  QWidget * parent_widget = nullptr;
  QWidget * widget = nullptr;
  bool taken = false;
  int key = 0;

  static constexpr uint8_t MAX_ALIVE_COUNTER = 3; 
  uint8_t alive_counter = MAX_ALIVE_COUNTER; 

  const char * CATEGORY; 

};


#endif // COMPONENT_REQ_HANDLER_HPP