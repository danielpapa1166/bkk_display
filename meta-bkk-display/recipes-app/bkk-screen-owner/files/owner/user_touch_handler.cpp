#include "user_touch_handler.hpp"
#include "ads7846_controller.h"
#include <rbuflogd/logger.h>

UserTouchHandler::UserTouchHandler(
  touchscreen_callback_t callback, 
  void * ctx)
  : on_touch_callback(callback), callback_ctx(ctx) {
  
  int retval = ads7846_controller_init(
    &controller, 
    &config);

  if (retval != 0) {
    log_error("TS init", "Failed to initialize ADS7846 controller");
    controller = nullptr;
    return;
  }

  retval = ads7846_controller_set_irq_callback(
    controller, 
    irq_callback, 
    this);

  if (retval != 0) {
    log_error("TS init", "Failed to set IRQ callback for ADS7846 controller");
    ads7846_controller_deinit(controller);
    controller = nullptr;
    return;
  }

  retval = ads7846_controller_start_irq_listener(controller); 
  if (retval != 0) {
    log_error("TS init", "Failed to start IRQ listener for ADS7846 controller");
    ads7846_controller_deinit(controller);
    controller = nullptr;
    return;
  }

  log_debug("TS init", "UserTouchHandler initialized successfully");
}


void UserTouchHandler::irq_callback(
  ads7846_irq_event_t event, 
  uint64_t foo, 
  void * user_arg) {
  
  UserTouchHandler * self = static_cast<UserTouchHandler *>(user_arg);
  if (self == nullptr) {
    log_error("TS irq", "UserTouchHandler context is null");
    return;
  }

  if (self->on_touch_callback == nullptr) {
    log_error("TS irq", "Touch callback is null");
    return;
  }

  ts_event_en ts_event = (event == ADS7846_IRQ_EVENT_FALLING)
    ? TOUCHSCREEN_EVENT_TOUCHED
    : TOUCHSCREEN_EVENT_RELEASED;

  self->on_touch_callback(ts_event, self->callback_ctx);
}

