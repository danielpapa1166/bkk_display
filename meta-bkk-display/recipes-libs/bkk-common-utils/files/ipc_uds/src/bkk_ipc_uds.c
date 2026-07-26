#include "bkk_ipc_uds_server.h"
#include "bkk_ipc_uds_client.h"
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>


// ---------------------------------------------------------------------------
// internal function to run the server thread
// ---------------------------------------------------------------------------
static void * ipc_uds_server_thread_func(void * arg) {
  ipc_uds_server_t * server = (ipc_uds_server_t *)arg;
  if (server == NULL) {
    return NULL;
  }

  struct epoll_event events[10];

  while (1) {
    int num_events = epoll_wait(
      server->event_fd, 
      events, 
      10, 
      -1);

    if (num_events < 0) {
      return NULL;
    }

    for (int i = 0; i < num_events; ++i) {
      if (events[i].data.fd == server->sock_fd) {
        int client_fd = accept(server->sock_fd, NULL, NULL);
        if (client_fd < 0) {
          continue;
        }
        if (server->callback != NULL) {
          // dispatch the client request to the callback function
          server->callback(client_fd, server->user_data);
        }
      }
    }
  }

  return NULL;
}


// ---------------------------------------------------------------------------
// external server side functions
// ---------------------------------------------------------------------------

ipc_uds_err_t ipc_uds_server_init(
    ipc_uds_server_t * server, 
    const char * socket_path, 
    ipc_uds_callback_t callback,
    void * user_data) {

  
  unlink(socket_path); // Remove existing socket file if it exists

  // Create a UNIX domain socket
  server->sock_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if(server->sock_fd < 0) {
    return IPC_UDS_ERR_SOCKET_OPEN_FAILED;
  }

  struct sockaddr_un server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sun_family = AF_UNIX;
  strncpy(
    server_addr.sun_path, 
    socket_path, 
    sizeof(server_addr.sun_path) - 1);

  // Bind the socket to the specified path
  const int bind_res = bind(
    server->sock_fd,
    (struct sockaddr*)&server_addr,
    sizeof(server_addr));
  if(bind_res < 0) {
    close(server->sock_fd);
    return IPC_UDS_ERR_SOCKET_BIND_FAILED;
  }

  // Listen for incoming connections
  const int listen_res = listen(server->sock_fd, 32);
  if(listen_res < 0) {
    close(server->sock_fd);
    return IPC_UDS_ERR_SOCKET_LISTEN_FAILED;
  }

  // Create an epoll instance to monitor the socket for incoming connections
  server->event_fd = epoll_create1(0);
  if(server->event_fd < 0) {
    close(server->sock_fd);
    return IPC_UDS_ERR_OTHER;
  }

  struct epoll_event event;
  memset(&event, 0, sizeof(event));
  event.events = EPOLLIN;
  event.data.fd = server->sock_fd;
  const int ctl_res = epoll_ctl(
    server->event_fd,
    EPOLL_CTL_ADD,
    server->sock_fd,
    &event);

  if(ctl_res < 0) {
    close(server->sock_fd);
    close(server->event_fd);
    return IPC_UDS_ERR_OTHER;
  }

  server->callback = callback;
  server->user_data = user_data;


  // Create a thread to handle incoming connections
  const int thread_create_res = pthread_create(
    &server->thread_fd, 
    NULL, 
    ipc_uds_server_thread_func, 
    server);

  if(thread_create_res != 0) {
    close(server->sock_fd);
    close(server->event_fd);
    return IPC_UDS_ERR_OTHER;
  }
    
  return IPC_UDS_ERR_NONE;
}


// read data from the client socket
ipc_uds_err_t ipc_uds_receive_from_client(int client_fd, 
    void * buffer, size_t buffer_size) {
  ssize_t n = recv(client_fd, buffer, buffer_size, 0);
  if (n < 0) {
    return IPC_UDS_ERR_SOCKET_RECV_FAILED;
  }
  else {
    return IPC_UDS_ERR_NONE;
  }
}


// send data to the client socket
ipc_uds_err_t ipc_uds_send_response(int client_fd, 
    const void * buffer, size_t buffer_size) {
  ssize_t n = send(client_fd, buffer, buffer_size, 0);
  if (n < 0) {
    return IPC_UDS_ERR_SOCKET_SEND_FAILED;
  }
  else {
    return IPC_UDS_ERR_NONE;
  }
}


void ipc_uds_close_client(int client_fd) {
  close(client_fd);
}



ipc_uds_err_t ipc_uds_cleanup_server(ipc_uds_server_t * server) {
  if (server == NULL) {
    return IPC_UDS_ERR_OTHER;
  }

  // Close the socket and event file descriptors
  close(server->sock_fd);
  close(server->event_fd);

  // Cancel and join the server thread
  pthread_cancel(server->thread_fd);
  pthread_join(server->thread_fd, NULL);

  return IPC_UDS_ERR_NONE;
}



// ---------------------------------------------------------------------------
// external client side functions
// ---------------------------------------------------------------------------



ipc_uds_err_t ipc_uds_client_send_recv(
    const char * socket_path, 
    void * request, 
    size_t request_size,
    void * response, 
    size_t response_size) {

  int sock_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if (sock_fd < 0) {
    return IPC_UDS_ERR_SOCKET_OPEN_FAILED;
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

  const int conn_res = connect(
    sock_fd, 
    (struct sockaddr *)&addr, 
    sizeof(addr));

  if (conn_res < 0) {
    close(sock_fd);
    return IPC_UDS_ERR_SOCKET_CONNECT_FAILED;
  }
  
  ssize_t n = send(sock_fd, request, request_size, 0);
  if (n < 0) {
    close(sock_fd);
    return IPC_UDS_ERR_SOCKET_SEND_FAILED;
  }
  
  n = recv(sock_fd, response, response_size, 0);
  if (n < 0) {
    close(sock_fd);
    return IPC_UDS_ERR_SOCKET_RECV_FAILED;
  }

  close(sock_fd);
  return IPC_UDS_ERR_NONE;
}
