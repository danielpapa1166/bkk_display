#ifndef BKK_SCREEN_HPP
#define BKK_SCREEN_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <pthread.h>
#include "bkk_screen_common_priv_defs.hpp"


class BkkScreen : public QWidget
{
  Q_OBJECT

public:
  explicit BkkScreen(QWidget *parent = nullptr);
  ~BkkScreen();
  int start_receive_thread(); 

private: 
  typedef struct {
    bkk_screen_component_id_t component_id;
    QWidget * widget;
    bool taken;
    int key;
  } component_data_t;

  QVBoxLayout * layout = nullptr;
  QWidget * infoBar = nullptr;
  QWidget * contentWidget = nullptr;

  component_data_t components[BKK_SCREEN_COMPONENT_MAX] = {
    {BKK_SCREEN_COMPONENT_INFO_BAR, nullptr, false, -1}
  };

  void setup_base_ui();
  int uds_init(int * const event_fd, int * const server_fd);

  static void * receive_thread_func(void * ctx);
  int handle_client_request(int client_fd);
  bkk_screen_internal_uds_err_t handle_acq_comp_req(
    bkk_screen_component_id_t component_id, 
    bkk_screen_acquire_component_response_t * response);

  bkk_screen_internal_uds_err_t handle_set_data_req(
    const bkk_screen_uds_request_t * request,
    bkk_screen_uds_response_t * response);

  pthread_t receive_thread_fd = -1;
}; 


#endif // BKK_SCREEN_HPP