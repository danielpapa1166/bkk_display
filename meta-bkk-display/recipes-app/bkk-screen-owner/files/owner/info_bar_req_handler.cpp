#include "info_bar_req_handler.hpp"


InfoBarReqHdl::InfoBarReqHdl(QWidget *parent)
    : ComponentReqHdl(parent) {
  component_id = BKK_SCREEN_COMPONENT_INFO_BAR;
  widget = new QWidget(parent);
  taken = false;
  key = 42;

  widget->setFixedHeight(46);
}


int InfoBarReqHdl::update_component(
    void * request,
    bkk_screen_uds_response_t * response
) {
  if(request == nullptr || response == nullptr) {
    return static_cast<int>(BKK_SCREEN_INTERNAL_UDS_ERR_INVALID_PARAM);
  }

  bkk_screen_info_bar_data_t * info_bar_data = 
    static_cast<bkk_screen_info_bar_data_t *>(request);

  // for now just print the data to console:
  printf("Info Bar Data Received:\n");
  printf("Clock: %s\n", info_bar_data->clock);
  printf("Online Status: %s\n", 
    info_bar_data->online_status == BKK_SCREEN_ONLINE_STATUS_ONLINE ? "Online" : "Offline");

  //response->error_code = BKK_SCREEN_ERROR_NONE;
  return static_cast<int>(BKK_SCREEN_INTERNAL_UDS_ERR_NONE);
}