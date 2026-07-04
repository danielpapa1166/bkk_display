#ifndef TABLE_REQ_HANDLER_HPP
#define TABLE_REQ_HANDLER_HPP

#include <QWidget>
#include <QObject>
#include <QTableWidget>
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
  const char * const CATEGORY = "Table";
  QTableWidget * arrivalsTable = nullptr;
  void setup_ui();
  void populateTable(); 
};


#endif // TABLE_REQ_HANDLER_HPP