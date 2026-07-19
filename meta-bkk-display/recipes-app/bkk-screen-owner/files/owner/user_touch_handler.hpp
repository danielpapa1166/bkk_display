#ifndef USER_TOUCH_HANDLER_HPP
#define USER_TOUCH_HANDLER_HPP

#include "ads7846_controller.h"

typedef enum {
  TOUCHSCREEN_EVENT_TOUCHED,
  TOUCHSCREEN_EVENT_RELEASED
} ts_event_en;

typedef void (*touchscreen_callback_t)(ts_event_en event, void * arg); 

struct UserTouchHandler {
  
  UserTouchHandler(touchscreen_callback_t callback, void * ctx);

private:

  static void irq_callback(
    ads7846_irq_event_t event, uint64_t foo, void * user_arg); 

  touchscreen_callback_t on_touch_callback;
  void * callback_ctx;

  ads7846_controller_t * controller = nullptr;

  const int screenWidth = 800;
  const int screenHeight = 480;
  const int adcRawMin = 200;
  const int adcRawMax = 3900;

  const ads7846_config_t config = {
    .spidev_path       = "/dev/spidev0.1",
    .spi_speed_hz      = 1000000,
    .spi_mode          = 0,
    .spi_bits_per_word = 8,
    .irq_gpio_number   = 25,
  };



};



#endif 