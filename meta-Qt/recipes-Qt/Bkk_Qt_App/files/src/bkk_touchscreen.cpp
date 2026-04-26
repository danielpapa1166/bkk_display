#include "bkk_touchscreen.hpp"
#include "ads7846_controller.h"
#include "bkk_elapsed_timer.hpp"
#include <algorithm>
#include <QString>


BkkTouchScreenWorker::BkkTouchScreenWorker(
    touchscreen_callback_t callback, void * user_arg) 
    : mainWindowCallback(callback), mainWindowCallbackArg(user_arg) {

  int retVal; 

  rbuflogd_producer_open(&loggerProducer, "TouchScreen"); 

  retVal = ads7846_controller_init(
    &controller, 
    &config); 

  if(retVal != 0) {
    rbuflogd_producer_log(
      &loggerProducer, 
      RBUF_LOG_LEVEL_ERROR, 
      "Init", 
      QString("Init failed. Error code: %1").arg(retVal).toStdString().c_str());

    errorStatus = TOUCHSCREEN_ERROR_INIT_FAILED;
    return; 
  }

  retVal = ads7846_controller_set_irq_callback(
    controller, 
    &BkkTouchScreenWorker::irq_callback, 
    this);

  if(retVal != 0) {
    rbuflogd_producer_log(
      &loggerProducer, 
      RBUF_LOG_LEVEL_ERROR, 
      "Init", 
      QString("Failed to set touchscreen IRQ callback. Error code: %1")
        .arg(retVal).toStdString().c_str());
    ads7846_controller_deinit(controller);

    errorStatus = TOUCHSCREEN_ERROR_IRQ_CALLBACK_FAILED;
    return;
  }

  retVal = ads7846_controller_start_irq_listener(controller);

  if(retVal != 0) {
    rbuflogd_producer_log(
      &loggerProducer, 
      RBUF_LOG_LEVEL_ERROR, 
      "Init", 
      QString("Failed to start touchscreen IRQ listener. Error code: %1")
        .arg(retVal).toStdString().c_str());
    ads7846_controller_deinit(controller);
    errorStatus = TOUCHSCREEN_ERROR_IRQ_LISTENER_FAILED;
    return;
  } 

  rbuflogd_producer_log(
    &loggerProducer, 
    RBUF_LOG_LEVEL_INFO, 
    "Init", 
    "Touchscreen controller initialized successfully");
}


BkkTouchScreenWorker::~BkkTouchScreenWorker() {
  if (controller != nullptr) {
    ads7846_controller_stop_irq_listener(controller);
    ads7846_controller_deinit(controller);
    rbuflogd_producer_log(
      &loggerProducer, 
      RBUF_LOG_LEVEL_INFO, 
      "DeInit", 
      "Touchscreen controller deinitialized");
  }

  rbuflogd_producer_close(&loggerProducer);
}



int BkkTouchScreenWorker::fetch_touch_coordinates(void) {

  if(controller == nullptr) {
    rbuflogd_producer_log(
      &loggerProducer, 
      RBUF_LOG_LEVEL_ERROR, 
      "Fetch", 
      "Cannot read ADC: Controller not initialized");
    return -1;
  }

  uint16_t x_raw = 0, y_raw = 0;

  int retVal; 

  uint64_t elapsedTime = BkkElapsedTimer::measureMs([&]() {
    retVal = ads7846_controller_fetch_touch_coords(
      controller, &x_raw, &y_raw);
  });


  if (retVal != 0) {
    rbuflogd_producer_log(
      &loggerProducer, 
      RBUF_LOG_LEVEL_ERROR, 
      "Fetch", 
      QString("Failed to read touch coordinates. Error code: %1")
        .arg(retVal).toStdString().c_str());
    errorStatus = TOUCHSCREEN_ERROR_SPI_READ_FAILED;
    return -1;
  }

  // convert to screen pixels, clamping to the calibrated ADC range:
  int xClamped = std::clamp(static_cast<int>(x_raw), adcRawMin, adcRawMax);
  int yClamped = std::clamp(static_cast<int>(y_raw), adcRawMin, adcRawMax);
  x_px = (xClamped - adcRawMin) * screenWidth  / (adcRawMax - adcRawMin);
  y_px = (yClamped - adcRawMin) * screenHeight / (adcRawMax - adcRawMin);

  return 0;
}


void BkkTouchScreenWorker::irq_callback(
  ads7846_irq_event_t event, uint64_t foo, void * user_arg) {

  (void)foo; // Unused parameter

  BkkTouchScreenWorker *self = static_cast<BkkTouchScreenWorker *>(user_arg);
  if (self == nullptr) {
    return;
  }

  if(event != ADS7846_IRQ_EVENT_FALLING 
      && event != ADS7846_IRQ_EVENT_RISING) {
    rbuflogd_producer_log(
      &self->loggerProducer, 
      RBUF_LOG_LEVEL_WARNING, 
      "Irq", 
      QString("Received unknown IRQ event: %1").arg(event).toStdString().c_str());
    return;
  }

  ts_event_en touchEvent = (event == ADS7846_IRQ_EVENT_FALLING) 
    ? TOUCHSCREEN_EVENT_TOUCHED 
    : TOUCHSCREEN_EVENT_RELEASED;

  if (self->mainWindowCallback != nullptr) {
    self->mainWindowCallback(
      touchEvent,  
      self->mainWindowCallbackArg);
  }
}
