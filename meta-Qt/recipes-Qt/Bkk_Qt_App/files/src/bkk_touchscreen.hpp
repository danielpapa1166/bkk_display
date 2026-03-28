#ifndef BKK_TOUCHSCREEN_HPP
#define BKK_TOUCHSCREEN_HPP

#include "ads7846_controller.h"


typedef enum {
  TOUCHSCREEN_ERROR_NONE = 0,
  TOUCHSCREEN_ERROR_INIT_FAILED = -1,
  TOUCHSCREEN_ERROR_IRQ_CALLBACK_FAILED = -2,
  TOUCHSCREEN_ERROR_IRQ_LISTENER_FAILED = -3,
  TOUCHSCREEN_ERROR_SPI_READ_FAILED = -4
} ts_error_en; 

typedef enum {
  TOUCHSCREEN_EVENT_TOUCHED,
  TOUCHSCREEN_EVENT_RELEASED
} ts_event_en;

typedef void (*touchscreen_callback_t)(ts_event_en event, void * arg); 


struct BkkTouchScreenWorker {
  BkkTouchScreenWorker(touchscreen_callback_t callback, void * user_arg); 
  ~BkkTouchScreenWorker();
  int fetch_touch_coordinates(void); 
  int getX() const { return x_px; }
  int getY() const { return y_px; }

private: 
  ts_error_en errorStatus = TOUCHSCREEN_ERROR_NONE;
  int x_px = 0, y_px = 0;
  ads7846_controller_t *controller = nullptr;
  const ads7846_config_t config = {
    .spidev_path       = "/dev/spidev0.1",
    .spi_speed_hz      = 1000000,
    .spi_mode          = 0,
    .spi_bits_per_word = 8,
    .irq_gpio_number   = 25,
  };
  
  touchscreen_callback_t mainWindowCallback = nullptr; 
  void * mainWindowCallbackArg = nullptr;  
  static void irq_callback(
    ads7846_irq_event_t event, uint64_t foo, void * user_arg); 

  const int screenWidth = 800;
  const int screenHeight = 480;
  const int adcDivider = 4095; 

};

#endif // BKK_TOUCHSCREEN_HPP