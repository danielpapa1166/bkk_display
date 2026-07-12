#include "component_req_handler.hpp"
#include "bkk_screen_client/common_defs.hpp"
#include <rbuflogd/logger.h>
#include <cstdlib>


void ComponentReqHdl::state_machine_transition(ComponentState_t new_state) {
  log_info(CATEGORY, 
    ("    Transitioning from state " 
      + get_state_name(state)
      + " >>> to state >>> " 
      + get_state_name(new_state)).c_str());
  state = new_state;
}


// ----------------------------------------------------------------------------
// client request handlers
// ----------------------------------------------------------------------------

bkk_screen_error_code_t ComponentReqHdl::handle_request(
    bkk_screen_uds_message_t * request, 
    bkk_screen_uds_message_t * response) {

  if(request == nullptr || response == nullptr) {
    return BKK_SCREEN_ERROR_INVALID_PARAM;
  }

  switch(request->header.cmd_id) {
    case BKK_SCREEN_COMMAND_ACQUIRE_COMPONENT: {

      const bkk_screen_error_code_t acquire_res 
        = acquire_component(request, response);

      if (acquire_res != BKK_SCREEN_ERROR_NONE) {
        log_warning(CATEGORY,
          ("Failed to acquire component " 
            + get_component_name()
            + ", error code: " 
            + std::to_string(acquire_res)).c_str());
      }
      else {
        log_info(CATEGORY,
          ("Component " + get_component_name() 
            + " acquired with key: " 
            + std::to_string(response->acquire_resp.key)).c_str()
        );
      }
      return acquire_res;
    }

    case BKK_SCREEN_COMMAND_PING: {
      const bkk_screen_error_code_t ping_res 
        = handle_ping_request(request, response);

      return ping_res;
    }
    case BKK_SCREEN_COMMAND_RELEASE_COMPONENT: {

      const bkk_screen_error_code_t release_res
        = handle_release_request(request, response);

      if (release_res != BKK_SCREEN_ERROR_NONE) {
        log_warning(CATEGORY,
          ("Failed to handle release for component "
            + get_component_name()
            + ", error code: " 
            + std::to_string(release_res)).c_str());
      }
      else {
        log_info(CATEGORY,
          ("Release handled successfully for component "
            + get_component_name()).c_str());
      }
      
      return release_res;
    }
    case BKK_SCREEN_COMMAND_SET_DATA: {

      alive_counter = MAX_ALIVE_COUNTER; // Reset alive counter on data update
      const bkk_screen_error_code_t update_res 
        = update_component(request, response);

      if (update_res != BKK_SCREEN_ERROR_NONE) {
        log_warning(CATEGORY,
          ("Failed to update component " 
            + get_component_name()
            + ", error code: " 
            + std::to_string(update_res)).c_str());
      }
      else {
        log_info(CATEGORY,
          ("Component " + get_component_name() 
            + " updated successfully").c_str()
        );
      }

      return update_res;
    }
    default: {
      log_warning(CATEGORY,
        ("Unknown command ID: " 
          + std::to_string(request->header.cmd_id)).c_str());
      return BKK_SCREEN_ERROR_INVALID_PARAM;
    }
  }
}


bkk_screen_error_code_t ComponentReqHdl::acquire_component(
    bkk_screen_uds_message_t * request, 
    bkk_screen_uds_message_t * response) {


  if(request == nullptr || response == nullptr) {
    log_error(CATEGORY, 
      "Invalid parameters: request or response is null");
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

  if(state == ComponentState::Acquired) {
    response->acquire_resp.error_code 
      = BKK_SCREEN_ERROR_COMPONENT_ALREADY_ACQUIRED;

    log_warning(CATEGORY, 
      ("Component " + std::to_string(component_id) 
      + " is already taken").c_str());
    
    return response->acquire_resp.error_code;
  }

  qt_thread_init_ui(); // Ensure UI is initialized on the Qt thread

  key = rand();
  taken = true;

  state_machine_transition(ComponentState::Acquired);
  

  response->acquire_resp.key = key;
  response->acquire_resp.error_code = BKK_SCREEN_ERROR_NONE;

  log_info(CATEGORY, 
    ("Component " 
      + std::to_string(component_id) 
      + " acquired with key: " 
      + std::to_string(key)).c_str());

  return response->acquire_resp.error_code;
}


bkk_screen_error_code_t ComponentReqHdl::handle_ping_request(    
    bkk_screen_uds_message_t * request,
    bkk_screen_uds_message_t * response
  ) {

  if(request == nullptr || response == nullptr) {
    log_error(CATEGORY, 
      "Invalid parameters: request or response is null");
    return BKK_SCREEN_ERROR_INVALID_PARAM;
  }
  
  if(state != ComponentState::Acquired) {
    log_warning(CATEGORY,
      ("Ping received for component " 
        + std::to_string(component_id) 
        + " which is not taken").c_str());

    response->header.cmd_id = request->header.cmd_id;
    response->generic_resp.error_code 
      = BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND;
    return BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND;
  }

  alive_counter = MAX_ALIVE_COUNTER;

  response->header.component_id = request->header.component_id;
  response->header.cmd_id = request->header.cmd_id;
  response->generic_resp.error_code = BKK_SCREEN_ERROR_NONE;

  return BKK_SCREEN_ERROR_NONE;
}


bkk_screen_error_code_t ComponentReqHdl::handle_release_request(    
    bkk_screen_uds_message_t * request,
    bkk_screen_uds_message_t * response
  ) {

  if(request == nullptr || response == nullptr) {
    log_error(CATEGORY, 
      "Invalid parameters: request or response is null");
    return BKK_SCREEN_ERROR_INVALID_PARAM;
  }
  
  if(state != ComponentState::Acquired) {
    log_warning(CATEGORY,
      ("Release received for component " 
        + std::to_string(component_id) 
        + " which is not taken").c_str());

    response->header.cmd_id = request->header.cmd_id;
    response->generic_resp.error_code 
      = BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND;
    return BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND;
  }

  state_machine_transition(ComponentState::Expiring);

  qt_thread_clear_component(); // Clear the component on the Qt thread

  response->header.component_id = request->header.component_id;
  response->header.cmd_id = request->header.cmd_id;
  response->generic_resp.error_code = BKK_SCREEN_ERROR_NONE;

  return BKK_SCREEN_ERROR_NONE;
}


int ComponentReqHdl::cyclic_alive_check() {
  if(state != ComponentState::Acquired) {
    return -1; // Not taken, no need to check
  }

  if(alive_counter > 0) {
    --alive_counter;
  }

  if(alive_counter == 0) {
    state_machine_transition(ComponentState::Expiring);
  }

  return alive_counter;
}


// ----------------------------------------------------------------------------
// UI management functions
// ----------------------------------------------------------------------------


void ComponentReqHdl::qt_thread_init_ui() {
  if (QThread::currentThread() == thread()) {
    init_ui();
    return;
  }
  QMetaObject::invokeMethod(
    this, "invoke_init_ui", Qt::BlockingQueuedConnection);
}


void ComponentReqHdl::qt_thread_refresh_ui() {
  if (QThread::currentThread() == thread()) {
    refresh_ui();
    return;
  }
  QMetaObject::invokeMethod(
    this, "invoke_refresh_ui", Qt::QueuedConnection);
}


void ComponentReqHdl::qt_thread_clear_component() {
  log_info(CATEGORY, 
    ("Clearing component " + get_component_name()).c_str());
  taken = false;
  key = -1; 
  alive_counter = MAX_ALIVE_COUNTER;

  if (QThread::currentThread() == thread()) {
    if (widget != nullptr) {
      delete widget;
      widget = nullptr;
    }
    state_machine_transition(ComponentState::Empty);
    return;
  }
  QMetaObject::invokeMethod(this, [this]() {
    if (widget != nullptr) {
      delete widget;
      widget = nullptr;
    }
    state_machine_transition(ComponentState::Empty);
  }, Qt::BlockingQueuedConnection);

  log_info(CATEGORY, 
    ("Component " + get_component_name() + " cleared").c_str());
}


// ----------------------------------------------------------------------------
// Getters
// ----------------------------------------------------------------------------

std::string ComponentReqHdl::get_component_name() const {
  switch (component_id) {
    case BKK_SCREEN_COMPONENT_INFO_BAR:
      return "INFO_BAR";
    case BKK_SCREEN_COMPONENT_STATUS_SCREEN:
      return "STATUS_SCREEN";
    case BKK_SCREEN_COMPONENT_TABLE:
      return "TABLE";
    default:
      return "UNKNOWN_COMPONENT";
  }
}

std::string ComponentReqHdl::get_state_name(ComponentState_t st) {
  switch (st) {
    case ComponentState::Empty:
      return "EMPTY";
      case ComponentState::Ready:
        return "READY";
    case ComponentState::Acquired:
      return "ACQUIRED";
    case ComponentState::Expiring:
      return "EXPIRING";
    case ComponentState::Dead: 
        return "DEAD";
    default:
      return "UNKNOWN_STATE";
  }
}