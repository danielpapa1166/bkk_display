#ifndef TABLE_REQ_HANDLER_HPP
#define TABLE_REQ_HANDLER_HPP

#include <QWidget>
#include <QObject>
#include <QTableWidget>
#include <vector>
#include "bkk_screen_common_priv_defs.hpp"
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

  QWidget *createDepartureCell(
    int departsInMin, const QColor &backgroundColor) const;
  void populateTable();


  // config constants:
  static constexpr int kMaxRows = 8;
  static constexpr int kBlinkThresholdGreen = 10; 
  static constexpr int kBlinkThresholdRed = 5; 

  static constexpr int kBlinkIntervalMs = 800; // blink interval 
  static constexpr int kFetchIntervalMs = 5000; // fetch new data 

};


#endif // TABLE_REQ_HANDLER_HPP