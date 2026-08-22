#include "status_screen_req_handler.hpp"
#include "bkk_screen_client/common_defs.hpp"
#include <QHBoxLayout>
#include <QLabel>
#include <rbuflogd/logger.h>


StatusScreenReqHdl::StatusScreenReqHdl(QWidget * parent)
  : ComponentReqHdl(parent) {

  CATEGORY = "StatScr";
  log_info(CATEGORY, "Initializing StatusScreenReqHdl");
  component_id = BKK_SCREEN_COMPONENT_STATUS_SCREEN;

  taken = false;
  key = 44;



  qt_thread_init_ui();

  log_info(CATEGORY, "StatusScreenReqHdl initialized");
}


void StatusScreenReqHdl::init_ui() {
  if (!is_Qt_thread()) {
    log_warning(CATEGORY, "init_ui() called from non-Qt thread");
    return;
  }
  // Implement the UI setup for the status screen component here
  widget = new QWidget(parent_widget);
  auto * layout = new QHBoxLayout(widget);

  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  widget->setLayout(layout);
  statusLabel = new QLabel(widget);
  statusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  statusLabel->setText(" . ");
  layout->addWidget(statusLabel);

  state_machine_transition(ComponentState::Ready);
}


void StatusScreenReqHdl::refresh_ui() {

  if (!is_Qt_thread()) {
    log_warning(CATEGORY, "refresh_ui() called from non-Qt thread");
    return;
  }

  if (statusLabel == nullptr) {
    log_error(CATEGORY, "UI label is not initialized");
    return;
  }

  statusLabel->setText(config_data.status_text);
}


bkk_screen_error_code_t StatusScreenReqHdl::update_component(
    bkk_screen_uds_message_t * request,
    bkk_screen_uds_message_t * response
) {
  if(request == nullptr || response == nullptr) {
    return BKK_SCREEN_ERROR_INVALID_PARAM;
  }

  if(state != ComponentState::Acquired) {
    response->header.component_id = request->header.component_id;
    response->header.cmd_id = request->header.cmd_id;
    response->generic_resp.error_code = BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND;
    log_warning(CATEGORY,
      ("Update request received for component "
        + get_component_name()
        + " which is not taken").c_str());
    return BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND;
  }


  if(request->header.cmd_id == BKK_SCREEN_COMMAND_SET_DATA) {

    config_data = request->set_status_screen_data;

  }

  qt_thread_refresh_ui();

  log_info(CATEGORY, "Updating status screen component");

  response->header.component_id = request->header.component_id;
  response->header.cmd_id = request->header.cmd_id;
  response->generic_resp.error_code = BKK_SCREEN_ERROR_NONE;

  return BKK_SCREEN_ERROR_NONE;
}