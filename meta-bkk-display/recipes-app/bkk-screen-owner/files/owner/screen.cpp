#include <unistd.h>
#include "screen.hpp"
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


BkkScreen::BkkScreen(QWidget *parent)
    : QWidget(parent)
{
  info_bar_handler = new InfoBarReqHdl(this);

  // depends on boot mode which to instanctiate: 
  main_content_handler = new TableReqHdl(this);

  setup_base_ui();

}

BkkScreen::~BkkScreen() {
  pthread_cancel(receive_thread_fd);
  pthread_join(receive_thread_fd, nullptr);
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

  layout->addWidget(info_bar_handler->get_widget(), 0, Qt::AlignTop);
  layout->addStretch(1);
  layout->addWidget(main_content_handler->get_widget(), 1);
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
    log_info(CATEGORY, "Received acquire request for INFO_BAR component");
    int res = info_bar_handler->handle_request(&request, &uds_response);
    (void) res;
  } 
  else {
    uds_response.acquire_resp.error_code = BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND;
    log_warning(CATEGORY, 
      ("Received acquire request for unknown component ID: " 
        + std::to_string(request.header.component_id)).c_str());
  }

  send(client_fd, &uds_response, sizeof(uds_response), 0);


  ::close(client_fd);
  return BKK_SCREEN_ERROR_NONE;
}