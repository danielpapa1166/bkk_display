#include <unistd.h>
#include "screen.hpp"
#include "bkk_screen_common_priv_defs.hpp"
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
  receive_thread_ctx_t * thread_ctx = static_cast<receive_thread_ctx_t *>(ctx);
  if (thread_ctx == nullptr) {
    return nullptr;
  }

  BkkScreen * screen = thread_ctx->screen;
  int sock_fd = thread_ctx->sock_fd;
  int event_fd = thread_ctx->event_fd;


  epoll_event events[MAX_EVENTS];

  while(1) {
    int num_events = epoll_wait(
      event_fd, events, 
      MAX_EVENTS, 
      -1); 

    if(num_events < 0) {
      printf("Failed to wait for events\n");
      return nullptr;
    }

    for (int i = 0; i < num_events; ++i) {
      if(events[i].data.fd == sock_fd) {
        int client_fd = accept(sock_fd, nullptr, nullptr);
        if(client_fd < 0) {
          printf("Failed to accept client connection\n");
          continue;
        }
        screen->dispatch_client_request(client_fd);
      }
    }
  }

  return nullptr;
}


BkkScreen::BkkScreen(QWidget *parent)
    : QWidget(parent)
{
  printf("BkkScreen constructor called\n");
  
  info_bar_handler = new InfoBarReqHdl(this);

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

  printf("Adding info bar widget to layout\n");
  layout->addWidget(info_bar_handler->get_widget(), 0);

  /*contentWidget = new QWidget(this);
  layout->addWidget(contentWidget, 1);*/ 
}

int BkkScreen::uds_init(int * const event_fd, int * const server_fd) {
  // note: 
  // same implementation as it is in bkk_uds_server/main.cpp, 
  // todo: create a common function for this in a shared header file to avoid code duplication

  unlink(BKK_SCREEN_UDS_NAME); // Remove existing socket file if it exists
  
  *server_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0); 
  if(*server_fd < 0) {
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
    return 1;
  }

  const int listen_res = listen(*server_fd, 32);
  if(listen_res < 0) {
    return 1;
  }

  printf("Server is listening on %s\n", BKK_SCREEN_UDS_NAME);

  *event_fd = epoll_create1(0);
  if(*event_fd < 0) {
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
    return 1;
  }

  return 0; 
}

int BkkScreen::start_receive_thread() {

  int sock_fd = -1;
  int event_fd = -1;


  const int uds_init_res = uds_init(&event_fd, &sock_fd);
  if(uds_init_res != 0) {
    return -1;
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
    return -1;
  }

  return 0;
}


int BkkScreen::dispatch_client_request(int client_fd) {
  bkk_screen_uds_request_t request {};
  int n = recv(
    client_fd, 
    &request, 
    sizeof(request), 
    0); 

  if (n != sizeof(request)) {
    printf("Failed to receive data from client\n");
    return -1;
  }

  bkk_screen_uds_response_t uds_response {};

  (void)request.cmd_id; 
  if(request.cmd_id == BKK_SCREEN_COMMAND_ACQUIRE_COMPONENT) {
    bkk_screen_acquire_component_request_t * acquire_req =
      reinterpret_cast<bkk_screen_acquire_component_request_t *>(request.payload);
    (void)acquire_req->component_id; 

    bkk_screen_acquire_component_response_t acq_response {};

    /*bkk_screen_internal_uds_err_t acq_res = handle_acq_comp_req(
      acquire_req->component_id, &acq_response);*/

    int res = info_bar_handler->acquire_component(acquire_req, &acq_response);

    acq_response.error_code = static_cast<bkk_screen_error_code_t>(res);
    memcpy(uds_response.payload, &acq_response, sizeof(acq_response));

    send(client_fd, &uds_response, sizeof(uds_response), 0);
  } 
  else if(request.cmd_id == BKK_SCREEN_COMMAND_RELEASE_COMPONENT) {
    // Placeholder for future implementation
  } 
  else if(request.cmd_id == BKK_SCREEN_COMMAND_SET_INFO_BAR_DATA) {
    bkk_screen_info_bar_data_t * info_bar_data =
      reinterpret_cast<bkk_screen_info_bar_data_t *>(request.payload);
    (void)info_bar_data->clock; 
    /*bkk_screen_internal_uds_err_t set_data_res = handle_set_data_req(
      &request, &uds_response);*/

    int res = info_bar_handler->update_component(info_bar_data, &uds_response);
    //uds_response.error_code = static_cast<bkk_screen_error_code_t>(res);
    send(client_fd, &uds_response, sizeof(uds_response), 0);
  } 
  else {
    printf("Unknown command ID received: %d\n", request.cmd_id);
    return -1;
  }


  ::close(client_fd);
  return 0;
}