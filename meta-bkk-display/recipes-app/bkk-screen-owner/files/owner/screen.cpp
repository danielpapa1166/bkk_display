#include <bkk_utils/bkk_ipc_uds_common_defs.h>
#include <unistd.h>
#include "screen.hpp"
#include <QMetaObject>
#include <QThread>
#include "component_req_handler.hpp"
#include "status_screen_req_handler.hpp"
#include "table_req_handler.hpp"
#include "user_touch_handler.hpp"

#include "bkk_screen_common_priv_defs.hpp"

#include <rbuflogd/logger.h>
#include <bkk_utils/bkk_ipc_uds_server.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <thread>
#include <unistd.h>
#include <iostream>


int BkkScreen::alive_check(void * ctx) {
  static const char * const CAT = "AliveChk";
  BkkScreen * screen = static_cast<BkkScreen *>(ctx);
  if (screen == nullptr) {
    log_error(CAT, "Alive check thread context is null");
    return -1;
  }

  if (screen->main_content_handler != nullptr) {
    const int res = screen->main_content_handler->cyclic_alive_check();
    if (res == 0) {
      log_warning(CAT, "Alive Counter 0, Clearing Main Content handler");
      screen->main_content_handler->qt_thread_clear_component();
    }
  }

  if(screen->info_bar_handler != nullptr) {
    const int res = screen->info_bar_handler->cyclic_alive_check();
    if(res == 0) {
      log_warning(CAT, "Alive Counter 0, Clearing Info Bar handler");
      screen->info_bar_handler->qt_thread_clear_component();
    }
  }

  return 0;
}


int BkkScreen::screen_pwr_off_callback(void * ctx) {
  static const char * const CAT = "ScrPwrOff";
  BkkScreen * self = static_cast<BkkScreen *>(ctx);
  if (self == nullptr) {
    log_error(CAT, "Screen power off callback context is null");
    return -1;
  }

  log_debug(CAT, "Screen power off timer expired, turning screen off");
  SCREEN_PWR_OFF();

  (void) bkk_stop_timer_with_callback(&self->screen_pwr_off_timer_ctx);
  (void) bkk_cleanup_timer_with_callback(&self->screen_pwr_off_timer_ctx);

  return 0;
}

void BkkScreen::screen_pwr_on_callback(ts_event_en event, void * ctx) {
  (void) event; 

  // handle only release event for now, here we want to turn the screen on when the user releases the touch
  if (event == TOUCHSCREEN_EVENT_RELEASED) {
    static const char * const CAT = "ScrPwrOn";
    BkkScreen * self = static_cast<BkkScreen *>(ctx);
    if (self == nullptr) {
      log_error(CAT, "Screen power on callback context is null");
      return;
    }
  
    log_debug(CAT, "Screen power on timer expired, turning screen on");
    SCREEN_PWR_ON();
  
    self->updateWidgets(); 

    if(self->screen_pwr_off_timer_ctx.is_running) {
      (void) bkk_stop_timer_with_callback(&self->screen_pwr_off_timer_ctx);
      (void) bkk_cleanup_timer_with_callback(&self->screen_pwr_off_timer_ctx);
    }
    // restart the screen power off timer
    timer_error_t timer_st = bkk_setup_timer_with_callback(
      &self->screen_pwr_off_timer_ctx);
    if (timer_st != TIMER_ERROR_NONE) {
      log_error(CAT, "Failed to setup screen power off timer");
    }
  }
  else {
    // handle everything else later
  }

  return;
}


BkkScreen::BkkScreen(QWidget *parent)
    : QWidget(parent)
{
  setup_base_ui();
  updateWidgets();

  timer_error_t timer_st = bkk_setup_timer_with_callback(
    &alive_check_thread_ctx);
  if (timer_st != TIMER_ERROR_NONE) {
    log_error(CATEGORY, "Failed to setup alive check timer"); 
  } 
  
  timer_st = bkk_setup_timer_with_callback(
    &screen_pwr_off_timer_ctx);
  if (timer_st != TIMER_ERROR_NONE) {
    log_error(CATEGORY, "Failed to setup screen power off timer");
  }

  touch_handler = new UserTouchHandler(screen_pwr_on_callback, this);
  
  const ipc_uds_err_t uds_init_res = ipc_uds_server_init(
    &uds_server, 
    BKK_SCREEN_UDS_NAME, 
    dispatch_client_request, 
    this);

  if(uds_init_res != IPC_UDS_ERR_NONE) {
    log_error(CATEGORY, (
      "Failed to initialize UDS server, error code: " 
      + std::to_string(uds_init_res)).c_str());
  }
}

BkkScreen::~BkkScreen() {
  pthread_cancel(receive_thread_fd);
  pthread_join(receive_thread_fd, nullptr);

  (void) bkk_cleanup_timer_with_callback(&alive_check_thread_ctx);
  (void) bkk_cleanup_timer_with_callback(&screen_pwr_off_timer_ctx);
}

ComponentReqHdl * BkkScreen::ensure_info_bar_handler() {
  if (info_bar_handler != nullptr) {
    return info_bar_handler;
  }
  
  if (QThread::currentThread() != thread()) {
    ComponentReqHdl * handler = nullptr;
    QMetaObject::invokeMethod(
      this,
      [this, &handler]() {
        // function recursively calls ensure_info_bar_handler 
        // on the correct thread: 
        handler = ensure_info_bar_handler();
      },
      Qt::BlockingQueuedConnection);
    return handler;
  }

  // running on the correct thread, create the info bar handler

  info_bar_handler = new InfoBarReqHdl(this);

  updateWidgets();

  return info_bar_handler;
}


ComponentReqHdl * BkkScreen::ensure_main_content_handler(
  bkk_screen_component_id_t component_id) {
 
  if (main_content_handler != nullptr
    && main_content_handler->get_component_id() == component_id) {

    // Already have the correct handler, return it
    return main_content_handler;
  }

  if (QThread::currentThread() != thread()) {
    ComponentReqHdl * handler = nullptr;
    QMetaObject::invokeMethod(
      this,
      [this, component_id, &handler]() {
        // function recursively calls ensure_main_content_handler 
        // on the correct thread: 
        handler = ensure_main_content_handler(component_id);
      },
      Qt::BlockingQueuedConnection);
    return handler;
  }

  if(main_content_handler != nullptr 
      && main_content_handler->get_state() != ComponentState::Empty) {
    log_warning(CATEGORY, 
      ("Main content handler is not empty, current state: " 
        + ComponentReqHdl::get_state_name(
          main_content_handler->get_state())).c_str());

    return nullptr;
  }

  if (main_content_handler != nullptr) {
    if (layout != nullptr && main_content_handler->get_widget() != nullptr) {
      layout->removeWidget(main_content_handler->get_widget());
    }

    delete main_content_handler;
    main_content_handler = nullptr;
  }

  switch (component_id) {
    case BKK_SCREEN_COMPONENT_STATUS_SCREEN:{
      main_content_handler = new StatusScreenReqHdl(this);
      break;
    }
    case BKK_SCREEN_COMPONENT_TABLE:{
      main_content_handler = new TableReqHdl(this);
      break;
    }
    default:
      return nullptr;
  }

  QThread::msleep(10); // Allow the UI to initialize

  while(main_content_handler->get_state() != ComponentState::Ready) {
    QThread::msleep(10);
  }

  updateWidgets();

  return main_content_handler;
}

void BkkScreen::setup_base_ui() {
  setMinimumSize(480, 320);
  setWindowTitle("BKK Display");

  setStyleSheet(
    "QWidget { background-color: #340a41; color: #ffffff; }"
    "QLabel  { background-color: #340a41; color: #ffffff; }"
    "QHeaderView::section { background-color: #505050; color: #ffffff; "
    "                        border: none; padding: 4px; font-weight: bold; }"
    "QTableWidget { background-color: #340a41; gridline-color: #505050;"
    "                border: none; }"
    "QTableCornerButton::section { background-color: #505050; }"
  );

  layout = new QVBoxLayout(this);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(6);
  layout->setAlignment(Qt::AlignTop);

}


void BkkScreen::updateWidgets() {
  if (QThread::currentThread() != thread()) {
    QMetaObject::invokeMethod(
      this,
      [this]() {
        // function recursively calls updateWidgets 
        // on the correct thread: 
        updateWidgets();
      },
      Qt::BlockingQueuedConnection);
    return;
  }

  // Clear existing widgets from the layout
  if (layout != nullptr) {
    while (QLayoutItem * item = layout->takeAt(0)) {
      delete item;
    }
  }

  if(info_bar_handler != nullptr) {
    layout->addWidget(info_bar_handler->get_widget(), 0, Qt::AlignTop);
  }

  if(main_content_handler != nullptr) {
    layout->addWidget(main_content_handler->get_widget(), 1);
  }
}


int BkkScreen::dispatch_client_request(int client_fd, void * user_data) {
  BkkScreen * self = static_cast<BkkScreen *>(user_data);
  if (self == nullptr) {
    log_error("Screen", "Dispatch client request context is null");
    return static_cast<int>(BKK_SCREEN_ERROR_OTHER);
  }
  bkk_screen_uds_message_t request {};
  ipc_uds_err_t n = ipc_uds_receive_from_client(
    client_fd, 
    &request, 
    sizeof(request)); 

  if (n != IPC_UDS_ERR_NONE) {
    log_error(self->CATEGORY, "Failed to receive data from client");
    return static_cast<int>(BKK_SCREEN_ERROR_OTHER);
  }

  bkk_screen_uds_message_t uds_response {};

  if(request.header.component_id == BKK_SCREEN_COMPONENT_INFO_BAR) {
    ComponentReqHdl * handler = self->ensure_info_bar_handler();
    if (handler == nullptr) {
      return static_cast<int>(BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND);
    }

    int res = handler->handle_request(&request, &uds_response);
    (void) res;
  } 
  else if(request.header.component_id == BKK_SCREEN_COMPONENT_STATUS_SCREEN) {
    ComponentReqHdl * handler = self->ensure_main_content_handler(
      BKK_SCREEN_COMPONENT_STATUS_SCREEN);
    if (handler == nullptr) {
      return static_cast<int>(BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND);
    }

    int res = handler->handle_request(&request, &uds_response);
    (void) res;
  }
  else if(request.header.component_id == BKK_SCREEN_COMPONENT_TABLE) {
    ComponentReqHdl * handler = self->ensure_main_content_handler(
      BKK_SCREEN_COMPONENT_TABLE);
    if (handler == nullptr) {
      return static_cast<int>(BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND);
    }

    int res = handler->handle_request(&request, &uds_response);
    (void) res;
  }
  else {
    uds_response.acquire_resp.error_code = static_cast<bkk_screen_error_code_t>(
      BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND
    );
    log_warning(self->CATEGORY, 
      ("Received acquire request for unknown component ID: " 
        + std::to_string(request.header.component_id)).c_str());
  }

  if(request.header.cmd_id == BKK_SCREEN_COMMAND_ACQUIRE_COMPONENT) {
    // todo: update the UI to reflect the newly acquired component
    // use retval of handle_request to determine if the acquisition was successful or not
    self->updateWidgets(); 
  }

  const ipc_uds_err_t send_res = ipc_uds_send_response(
    client_fd, &uds_response, sizeof(uds_response));

  if (send_res != IPC_UDS_ERR_NONE) {
    log_error(self->CATEGORY, (
      "Failed to send response to client, error code: "
      + std::to_string(send_res)).c_str());
  }

  ipc_uds_close_client(client_fd);
  
  return static_cast<int>(BKK_SCREEN_ERROR_NONE);
}