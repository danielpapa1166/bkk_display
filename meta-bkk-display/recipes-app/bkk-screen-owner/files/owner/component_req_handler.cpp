#include "component_req_handler.hpp"

int ComponentReqHdl::acquire_component(
    bkk_screen_acquire_component_request_t * request, 
    bkk_screen_acquire_component_response_t * response) {

  if(request == nullptr || response == nullptr) {
    return static_cast<int>(BKK_SCREEN_INTERNAL_UDS_ERR_INVALID_PARAM);
  }

  response->key = -1;
  response->component_id = request->component_id;
  
  if(request->component_id != component_id) {
    response->error_code = static_cast<bkk_screen_error_code_t>(
      BKK_SCREEN_INTERNAL_UDS_ERR_INVALID_PARAM
    );
    return static_cast<int>(response->error_code);
  }

  if(taken) {
    response->error_code = static_cast<bkk_screen_error_code_t>(
      BKK_SCREEN_INTERNAL_UDS_ERR_COMP_TAKEN
    );
    return static_cast<int>(response->error_code);
  }

  // generate a random key for the component: 
  key = 42;

  taken = true;
  response->key = key;
  response->error_code = BKK_SCREEN_ERROR_NONE;

  return static_cast<int>(BKK_SCREEN_INTERNAL_UDS_ERR_NONE);
}