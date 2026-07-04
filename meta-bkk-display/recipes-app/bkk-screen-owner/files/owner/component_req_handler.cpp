#include "component_req_handler.hpp"
#include <rbuflogd/logger.h>

bkk_screen_error_code_t ComponentReqHdl::handle_request(
    bkk_screen_uds_message_t * request, 
    bkk_screen_uds_message_t * response) {

  if(request == nullptr || response == nullptr) {
    return BKK_SCREEN_ERROR_INVALID_PARAM;
  }

  switch(request->header.cmd_id) {
    case BKK_SCREEN_COMMAND_ACQUIRE_COMPONENT:
      return acquire_component(request, response);
    case BKK_SCREEN_COMMAND_RELEASE_COMPONENT:
      // Implement release logic here
      return BKK_SCREEN_ERROR_NONE;
    case BKK_SCREEN_COMMAND_SET_INFO_BAR_DATA:
      return update_component(request, response);
    default:
      log_warning("CompReq", 
        ("Unknown command ID: " + std::to_string(request->header.cmd_id)).c_str());
      return BKK_SCREEN_ERROR_INVALID_PARAM;
  }
}


bkk_screen_error_code_t ComponentReqHdl::acquire_component(
    bkk_screen_uds_message_t * request, 
    bkk_screen_uds_message_t * response) {

  static const char * const CATEGORY = "CompReq";

  if(request == nullptr || response == nullptr) {
    log_error(CATEGORY, "Invalid parameters: request or response is null");
    return BKK_SCREEN_ERROR_INVALID_PARAM;
  }

  response->acquire_resp.key = -1;
  response->header.component_id = request->header.component_id;
  
  if(request->header.component_id != component_id) {
    response->acquire_resp.error_code 
      = BKK_SCREEN_ERROR_INVALID_PARAM;

    log_warning(CATEGORY, 
      ("Request for invalid component ID: " 
        + std::to_string(request->header.component_id)).c_str());

    return response->acquire_resp.error_code;
  }

  if(taken) {
    response->acquire_resp.error_code 
      = BKK_SCREEN_ERROR_COMPONENT_ALREADY_ACQUIRED;

    log_warning(CATEGORY, 
      ("Component " + std::to_string(component_id) 
      + " is already taken").c_str());
    
    return response->acquire_resp.error_code;
  }

  // generate a random key for the component: 
  key = 42;
  taken = true;

  response->acquire_resp.key = key;
  response->acquire_resp.error_code = BKK_SCREEN_ERROR_NONE;

  log_info(CATEGORY, 
    ("Component " 
      + std::to_string(component_id) 
      + " acquired with key: " 
      + std::to_string(key)).c_str());

  return response->acquire_resp.error_code;
}