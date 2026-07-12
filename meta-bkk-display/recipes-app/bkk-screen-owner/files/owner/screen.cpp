#include <unistd.h>
#include "screen.hpp"
#include <QMetaObject>
#include <QThread>
#include "component_req_handler.hpp"
#include "status_screen_req_handler.hpp"
#include "table_req_handler.hpp"

#include "bkk_screen_common_priv_defs.hpp"

#include <rbuflogd/logger.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <thread>
#include <unistd.h>
#include <iostream>

static const int MAX_EVENTS = 10;
typedef struct {
  int sock_fd;
  int event_fd;
  BkkScreen * screen;
} receive_thread_ctx_t;



#include <sys/timerfd.h>
#include <sys/select.h>

// these function should be moved to a common utility file
static int setup_timer(int time_sec, int * timer_fd, fd_set * readfds) {
  *timer_fd = timerfd_create(CLOCK_REALTIME, 0);
  if (*timer_fd < 0) {
    return -1;
  }

  struct itimerspec timerSpec;
  timerSpec.it_interval.tv_sec = time_sec; // Interval for periodic timer
  timerSpec.it_interval.tv_nsec = 0;
  timerSpec.it_value.tv_sec = time_sec; // Initial expiration
  timerSpec.it_value.tv_nsec = 0;

  const int res = timerfd_settime(
    *timer_fd, 
    0, 
    &timerSpec, 
    nullptr);

  if (res < 0) {
    return -1;
  }
  
  FD_ZERO(readfds);
  FD_SET(*timer_fd, readfds);
  return 0;
}

static int wait_on_timer(int timer_fd, fd_set * readfds) {
  int res = select(
    timer_fd + 1, 
    readfds, 
    nullptr, 
    nullptr, 
    nullptr);

  if (res < 0) {
    return -1;
  }

  char buf[8];
  ssize_t n = read(timer_fd, buf, sizeof(buf));
  if (n < 0) {
    return -1;
  }

  return 0;
}


void * BkkScreen::receive_thread_func(void * ctx) {
  static const char * const CAT = "RxThr";
  receive_thread_ctx_t * thread_ctx = static_cast<receive_thread_ctx_t *>(ctx);
  if (thread_ctx == nullptr) {
    log_error(CAT, "Receive thread context is null");
    return nullptr;
  }

  BkkScreen * screen = thread_ctx->screen;
  int sock_fd = thread_ctx->sock_fd;
  int event_fd = thread_ctx->event_fd;


  epoll_event events[MAX_EVENTS];

  log_info(CAT, "Receive thread started, waiting for events...");

  while(1) {
    int num_events = epoll_wait(
      event_fd, events, 
      MAX_EVENTS, 
      -1); 

    if(num_events < 0) {
      log_error(CAT, "epoll_wait failed");
      return nullptr;
    }

    for (int i = 0; i < num_events; ++i) {
      if(events[i].data.fd == sock_fd) {
        int client_fd = accept(sock_fd, nullptr, nullptr);
        if(client_fd < 0) {
          log_error(CAT, "Failed to accept client connection");
          continue;
        }
        bkk_screen_error_code_t res 
          = screen->dispatch_client_request(client_fd);

        (void) res;
      }
    }
  }

  return nullptr;
}

void * BkkScreen::alive_check_thread_func(void * ctx) {
  static const char * const CAT = "AliveChk";
  receive_thread_ctx_t * thread_ctx = static_cast<receive_thread_ctx_t *>(ctx);
  if (thread_ctx == nullptr) {
    log_error(CAT, "Alive check thread context is null");
    return nullptr;
  }

  BkkScreen * screen = thread_ctx->screen;

  int timer_fd = -1;
  fd_set readfds;

  if (setup_timer(1, &timer_fd, &readfds) < 0) {
    log_error(CAT, "Failed to set up timer");
    return nullptr;
  }

  log_info(CAT, "Alive check thread started, waiting for timer events...");

  while (1) {
    if (wait_on_timer(timer_fd, &readfds) < 0) {
      log_error(CAT, "Failed to wait on timer");
      continue;
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
  }

  return nullptr;
}


BkkScreen::BkkScreen(QWidget *parent)
    : QWidget(parent)
{
  setup_base_ui();
  updateWidgets();
}

BkkScreen::~BkkScreen() {
  pthread_cancel(receive_thread_fd);
  pthread_cancel(alive_check_thread_fd);
  pthread_join(receive_thread_fd, nullptr);
  pthread_join(alive_check_thread_fd, nullptr);
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
    log_info(CATEGORY, 
      ("-----ensure_main_content_handler called from non-Qt thread, "
        "dispatching to Qt thread for component ID: " 
        + std::to_string(component_id)).c_str());
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


  log_info(CATEGORY, 
    ("-----ON Qt thread: Creating new main content handler for component ID: " 
      + std::to_string(component_id)).c_str());

  if(main_content_handler != nullptr 
      && main_content_handler->get_state() != ComponentState::Empty) {
    log_warning(CATEGORY, 
      (">>>>>>>Main content handler is not empty, current state: " 
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
      log_info(CATEGORY, "------Creating StatusScreenReqHdl");
      main_content_handler = new StatusScreenReqHdl(this);
      break;
    }
    case BKK_SCREEN_COMPONENT_TABLE:{
      log_info(CATEGORY, "-----Creating TableReqHdl");
      main_content_handler = new TableReqHdl(this);
      break;
    }
    default:
      return nullptr;
  }

  log_info(CATEGORY, 
    ("-----Main content handler created for component ID: " 
      + std::to_string(component_id)).c_str());

  QThread::msleep(10); // Allow the UI to initialize

  while(main_content_handler->get_state() != ComponentState::Ready) {
    log_info(CATEGORY, 
      ("-----Waiting for main content handler to be ready, current state: " 
        + ComponentReqHdl::get_state_name(
          main_content_handler->get_state())).c_str());
    QThread::msleep(10);
  }

  log_info(CATEGORY, 
    ("-----Main content handler created for component ID: " 
      + std::to_string(component_id)
      + " updating widgets ").c_str());
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


int BkkScreen::uds_init(int * const event_fd, int * const server_fd) {
  // note: 
  // same implementation as it is in bkk_uds_server/main.cpp, 
  // todo: create a common function for this in a shared header file to avoid code duplication

  unlink(BKK_SCREEN_UDS_NAME); // Remove existing socket file if it exists
  
  *server_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0); 
  if(*server_fd < 0) {
    log_error(CATEGORY, "Failed to create UDS socket");
    return 1;
  }

  sockaddr_un server_addr {};
  server_addr.sun_family = AF_UNIX;
  strncpy(
    server_addr.sun_path, 
    BKK_SCREEN_UDS_NAME, 
    sizeof(server_addr.sun_path) - 1);
 
  const int bind_res = bind(
    *server_fd, 
    (sockaddr*)&server_addr, 
    sizeof(server_addr));

  if(bind_res < 0) {
    log_error(CATEGORY, "Failed to bind UDS socket");
    return 1;
  }

  const int listen_res = listen(*server_fd, 32);
  if(listen_res < 0) {
    log_error(CATEGORY, "Failed to listen on UDS socket");
    return 1;
  }

  *event_fd = epoll_create1(0);
  if(*event_fd < 0) {
    log_error(CATEGORY, "Failed to create epoll instance");
    return 1;
  }

  epoll_event event {};
  event.events = EPOLLIN;
  event.data.fd = *server_fd;

  const int ctl_res = epoll_ctl(
    *event_fd, 
    EPOLL_CTL_ADD, 
    *server_fd, 
    &event);
  if(ctl_res < 0) {
    log_error(CATEGORY, "Failed to add server fd to epoll");
    return 1;
  }

  log_info(CATEGORY, 
    ("UDS server is listening on socket: " 
      + std::string(BKK_SCREEN_UDS_NAME)).c_str());

  return 0; 
}

bkk_screen_error_code_t BkkScreen::start_receive_thread() {

  int sock_fd = -1;
  int event_fd = -1;


  const int uds_init_res = uds_init(
    &event_fd, &sock_fd);
  if(uds_init_res != 0) {
    log_error(CATEGORY, "Failed to initialize UDS server");
    return BKK_SCREEN_ERROR_OTHER;
  }

  receive_thread_ctx_t * thread_ctx = new receive_thread_ctx_t;
  thread_ctx->sock_fd = sock_fd;
  thread_ctx->event_fd = event_fd;
  thread_ctx->screen = this;
    
  const int thread_create_res = pthread_create(
    &receive_thread_fd, 
    nullptr, 
    receive_thread_func, 
    thread_ctx);

  if (thread_create_res != 0) {
    log_error(CATEGORY, "Failed to create receive thread");
    return BKK_SCREEN_ERROR_OTHER;
  }

  return BKK_SCREEN_ERROR_NONE;
}


bkk_screen_error_code_t BkkScreen::start_alive_check_thread() {
  receive_thread_ctx_t * thread_ctx = new receive_thread_ctx_t;
  thread_ctx->screen = this;

  const int thread_create_res = pthread_create(
    &alive_check_thread_fd, 
    nullptr, 
    alive_check_thread_func, 
    thread_ctx);

  if (thread_create_res != 0) {
    log_error(CATEGORY, "Failed to create alive check thread");
    return BKK_SCREEN_ERROR_OTHER;
  }

  return BKK_SCREEN_ERROR_NONE;
}


bkk_screen_error_code_t BkkScreen::dispatch_client_request(int client_fd) {
  bkk_screen_uds_message_t request {};
  int n = recv(
    client_fd, 
    &request, 
    sizeof(request), 
    0); 

  if (n != sizeof(request)) {
    log_error(CATEGORY, "Failed to receive data from client");
    return BKK_SCREEN_ERROR_OTHER;
  }

  bkk_screen_uds_message_t uds_response {};

  if(request.header.component_id == BKK_SCREEN_COMPONENT_INFO_BAR) {
    ComponentReqHdl * handler = ensure_info_bar_handler();
    if (handler == nullptr) {
      return BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND;
    }

    int res = handler->handle_request(&request, &uds_response);
    (void) res;
  } 
  else if(request.header.component_id == BKK_SCREEN_COMPONENT_STATUS_SCREEN) {
    ComponentReqHdl * handler = ensure_main_content_handler(
      BKK_SCREEN_COMPONENT_STATUS_SCREEN);
    if (handler == nullptr) {
      return BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND;
    }

    int res = handler->handle_request(&request, &uds_response);
    (void) res;
  }
  else if(request.header.component_id == BKK_SCREEN_COMPONENT_TABLE) {
    ComponentReqHdl * handler = ensure_main_content_handler(
      BKK_SCREEN_COMPONENT_TABLE);
    if (handler == nullptr) {
      return BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND;
    }

    int res = handler->handle_request(&request, &uds_response);
    (void) res;
  }
  else {
    uds_response.acquire_resp.error_code = BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND;
    log_warning(CATEGORY, 
      ("Received acquire request for unknown component ID: " 
        + std::to_string(request.header.component_id)).c_str());
  }

  if(request.header.cmd_id == BKK_SCREEN_COMMAND_ACQUIRE_COMPONENT) {
    // todo: update the UI to reflect the newly acquired component
    // use retval of handle_request to determine if the acquisition was successful or not
    updateWidgets(); 
  }

  send(client_fd, &uds_response, sizeof(uds_response), 0);


  ::close(client_fd);
  return BKK_SCREEN_ERROR_NONE;
}