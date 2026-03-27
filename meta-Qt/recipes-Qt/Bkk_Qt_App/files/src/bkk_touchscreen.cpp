#include "bkk_touchscreen.hpp"
#include "ads7846_controller.h"
#include "bkk_logger.hpp"


// these macros to be checked, and moved to the controller header: 
#define ADS7846_CMD_X  0xD0  /* channel 1 (X), 12-bit, differential */
#define ADS7846_CMD_Y  0x90  /* channel 0 (Y), 12-bit, differential */


BkkTouchScreen::BkkTouchScreen(touchscreen_callback_t callback) 
    : mainWindowCallback(callback) {

  int retVal; 

  retVal = ads7846_controller_init(
    &controller, 
    &config); 

  if(retVal != 0) {
    Logger::error("TouchScreenIF", 
      QString("Init failed. Error code: %1").arg(retVal));

    errorStatus = TOUCHSCREEN_ERROR_INIT_FAILED;
    return; 
  }

  retVal = ads7846_controller_set_irq_callback(
    controller, 
    &BkkTouchScreen::irq_callback, 
    this);

  if(retVal != 0) {
    Logger::error("TouchScreenIF", 
      QString("Failed to set touchscreen IRQ callback. Error code: %1")
        .arg(retVal));
    ads7846_controller_deinit(controller);

    errorStatus = TOUCHSCREEN_ERROR_IRQ_CALLBACK_FAILED;
    return;
  }

  retVal = ads7846_controller_start_irq_listener(controller);

  if(retVal != 0) {
    Logger::error("TouchScreenIF", 
      QString("Failed to start touchscreen IRQ listener. Error code: %1")
        .arg(retVal));
    ads7846_controller_deinit(controller);
    errorStatus = TOUCHSCREEN_ERROR_IRQ_LISTENER_FAILED;
    return;
  } 
}


BkkTouchScreen::~BkkTouchScreen() {
  if (controller != nullptr) {
    ads7846_controller_stop_irq_listener(controller);
    ads7846_controller_deinit(controller);
    Logger::info("TouchScreenIF", "Touchscreen controller deinitialized");
  }
}


int BkkTouchScreen::read_adc(uint8_t cmd, uint16_t * out) {

  uint8_t tx[] = { cmd, 0x00, 0x00 };
  uint8_t rx[3] = { 0 };

  int ret = ads7846_controller_spi_transfer(
    controller, tx, rx, sizeof(tx));
  if (ret != 0) {
    errorStatus = TOUCHSCREEN_ERROR_SPI_READ_FAILED;
    return ret;
  }

  *out = (uint16_t)((((rx[1] << 8) | rx[2]) >> 3) & 0x0FFF);
  return 0;
} 

int BkkTouchScreen::fetch_touch_coordinates(void) {

  if(controller == nullptr) {
    Logger::error("TouchScreenIF", 
      "Cannot read ADC: Controller not initialized");
    return -1;
  }

  uint16_t x_raw = 0, y_raw = 0;

  int retVal; 

  retVal = read_adc(ADS7846_CMD_X, &x_raw);
  retVal |= read_adc(ADS7846_CMD_Y, &y_raw);

  if (retVal != 0) {
    Logger::error("TouchScreenIF", 
      QString("Failed to read touch coordinates. Error code: %1")
        .arg(retVal));
    errorStatus = TOUCHSCREEN_ERROR_SPI_READ_FAILED;
    return -1;
  }

  x_pos = x_raw;
  y_pos = y_raw;

  return 0;
}


void BkkTouchScreen::irq_callback(
  ads7846_irq_event_t event, uint64_t foo, void * user_arg) {

  (void)foo; // Unused parameter

  BkkTouchScreen *self = static_cast<BkkTouchScreen *>(user_arg);
  if (self == nullptr) {
    return;
  }

  if (event == ADS7846_IRQ_EVENT_FALLING) {
    // Pen down — read touch coordinates
    if (self->fetch_touch_coordinates() == 0) {
      Logger::info("TouchScreenIF", 
        QString("Touch detected at X=%1, Y=%2").arg(self->x_pos).arg(self->y_pos));

      if (self->mainWindowCallback != nullptr) {
        self->mainWindowCallback(&self->x_pos, &self->y_pos);
      }
    }
  } else {
    Logger::info("TouchScreenIF", "Touch released");
  }
}
