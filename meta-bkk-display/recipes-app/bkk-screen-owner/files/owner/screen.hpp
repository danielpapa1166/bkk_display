#ifndef BKK_SCREEN_HPP
#define BKK_SCREEN_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <pthread.h>
#include "bkk_screen_common_priv_defs.hpp"
#include "info_bar_req_handler.hpp"


class BkkScreen : public QWidget
{
  Q_OBJECT

public:
  explicit BkkScreen(QWidget *parent = nullptr);
  ~BkkScreen();
  int start_receive_thread(); 

private: 


  QVBoxLayout * layout = nullptr;
  InfoBarReqHdl * info_bar_handler = nullptr;


  void setup_base_ui();
  int uds_init(int * const event_fd, int * const server_fd);

  static void * receive_thread_func(void * ctx);
  int dispatch_client_request(int client_fd);
  bkk_screen_internal_uds_err_t handle_acq_comp_req(
    bkk_screen_component_id_t component_id, 
    bkk_screen_acquire_component_response_t * response);

  bkk_screen_internal_uds_err_t handle_set_data_req(
    const bkk_screen_uds_request_t * request,
    bkk_screen_uds_response_t * response);

  pthread_t receive_thread_fd = -1;
}; 


#endif // BKK_SCREEN_HPP