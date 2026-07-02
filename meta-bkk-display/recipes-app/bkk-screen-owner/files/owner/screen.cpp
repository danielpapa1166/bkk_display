#include <unistd.h>
#include "screen.hpp"
#include "bkk_screen_common_priv_defs.hpp"
#include <pthread.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <thread>
#include <unistd.h>

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

        screen->handle_client_request(client_fd);
      }
    }
  }

  return nullptr;
}


BkkScreen::BkkScreen(QWidget *parent)
    : QWidget(parent)
{
  setup_base_ui();
}

BkkScreen::~BkkScreen() {
  if (receive_thread_fd != -1) {
    pthread_cancel(receive_thread_fd);
    pthread_join(receive_thread_fd, nullptr);
  }
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

  infoBar = new QWidget(this);
  infoBar->setFixedHeight(46);
  layout->addWidget(infoBar);

  contentWidget = new QWidget(this);
  layout->addWidget(contentWidget, 1);
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


int BkkScreen::handle_client_request(int client_fd) {

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

  (void)request.cmd_id; 
  if(request.cmd_id == BKK_SCREEN_COMMAND_ACQUIRE_COMPONENT) {
    bkk_screen_acquire_component_request_t * acquire_req =
      reinterpret_cast<bkk_screen_acquire_component_request_t *>(request.payload);
    (void)acquire_req->component_id; 
    printf("Received acquire component request for component ID: %d\n", acquire_req->component_id);
  } 
  else if(request.cmd_id == BKK_SCREEN_COMMAND_RELEASE_COMPONENT) {
    // Placeholder for future implementation
  } 
  else if(request.cmd_id == BKK_SCREEN_COMMAND_SET_INFO_BAR_DATA) {
    bkk_screen_info_bar_data_t * info_bar_data =
      reinterpret_cast<bkk_screen_info_bar_data_t *>(request.payload);
    (void)info_bar_data->clock; 
    printf("Received info bar data with clock: %s\n", info_bar_data->clock.c_str());
  } 
  else {
    printf("Unknown command ID received: %d\n", request.cmd_id);
    return -1;
  }


  ::close(client_fd);
  return 0;
}