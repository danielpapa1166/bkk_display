#ifndef TABLE_REQ_HANDLER_HPP
#define TABLE_REQ_HANDLER_HPP

#include <QWidget>
#include <QObject>
#include <QTableWidget>
#include <vector>
#include "bkk_screen_common_priv_defs.hpp"
#include <bkk_utils/bkk_utils_timing.h>
#include "component_req_handler.hpp"


class TableReqHdl : public ComponentReqHdl {
  Q_OBJECT
public:
  explicit TableReqHdl(QWidget *parent = nullptr);

  bkk_screen_error_code_t update_component(
    bkk_screen_uds_message_t * request,
    bkk_screen_uds_message_t * response
  ) override;

private: 
  QTableWidget * arrivalsTable = nullptr;
  std::vector<arrival_info_t> arrivals;

  void init_ui() override;
  void refresh_ui() override;
  void qt_thread_clear_component() override;

  QWidget *createLineIdCell(const arrival_info_t & arrival, 
    const QColor &backgroundColor) const;

  QWidget *createDepartureCell(
    int departsInMin, const QColor &backgroundColor) const;
  void populateTable();

  bool blinkState = false;
  static int blink_screen_thread_func(void * arg);
  static constexpr int kBlinkIntervalNs = 800000000; // blink interval 

  timer_thread_ctx_t blink_timer_ctx = {
    .config = {
      .timer_fd = -1,
      .cyclic_expiration_sec = 0, 
      .cyclic_expiration_nsec = kBlinkIntervalNs,
      .initial_expiration_sec = 0,
      .initial_expiration_nsec = kBlinkIntervalNs,
    },
    .callback = blink_screen_thread_func,
    .arg = this,
    .is_running = false,
    .thread_created = false,
    .thread_joined = false,
  };


  // config constants:
  static constexpr int kMaxRows = 8;
  static constexpr int kBlinkThresholdGreen = 10; 
  static constexpr int kBlinkThresholdRed = 5; 

  static constexpr int numOfStationCharsToDisplay = 16;
  static constexpr int numOfDestinationCharsToDisplay = 28;

};


#endif // TABLE_REQ_HANDLER_HPP