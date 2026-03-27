#ifndef BKK_TOUCHSCREEN_HPP
#define BKK_TOUCHSCREEN_HPP

#include "ads7846_controller.h"


typedef void (*touchscreen_callback_t)(int * x, int * y); 
typedef enum {
  TOUCHSCREEN_ERROR_NONE = 0,
  TOUCHSCREEN_ERROR_INIT_FAILED = -1,
  TOUCHSCREEN_ERROR_IRQ_CALLBACK_FAILED = -2,
  TOUCHSCREEN_ERROR_IRQ_LISTENER_FAILED = -3,
  TOUCHSCREEN_ERROR_SPI_READ_FAILED = -4
} ts_error_en; 


struct BkkTouchScreen
{
  BkkTouchScreen(touchscreen_callback_t callback); 
  ~BkkTouchScreen();


private: 
  ts_error_en errorStatus = TOUCHSCREEN_ERROR_NONE;
  int x_pos = 0, y_pos = 0;
  ads7846_controller_t *controller = nullptr;
  const ads7846_config_t config = {
    .spidev_path       = "/dev/spidev0.1",
    .spi_speed_hz      = 1000000,
    .spi_mode          = 0,
    .spi_bits_per_word = 8,
    .irq_gpio_number   = 25,
  };
  
  touchscreen_callback_t mainWindowCallback = nullptr;   
  
  static void irq_callback(
    ads7846_irq_event_t event, uint64_t foo, void * user_arg); 
  
  int read_adc(uint8_t command, uint16_t *out);
  int fetch_touch_coordinates(void);

};

#endif // BKK_TOUCHSCREEN_HPP