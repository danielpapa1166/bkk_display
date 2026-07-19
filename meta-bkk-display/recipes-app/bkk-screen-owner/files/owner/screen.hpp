#ifndef BKK_SCREEN_HPP
#define BKK_SCREEN_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <pthread.h>
#include <bkk_utils/bkk_utils_timing.h>
#include <bkk_utils/bkk_screen_backlight.h>
#include "bkk_screen_common_priv_defs.hpp"
#include "component_req_handler.hpp"
#include "info_bar_req_handler.hpp"
#include "user_touch_handler.hpp"


class BkkScreen : public QWidget
{
  Q_OBJECT

public:
  explicit BkkScreen(QWidget *parent = nullptr);
  ~BkkScreen();
  bkk_screen_error_code_t start_receive_thread(); 

private: 
  const char * const CATEGORY = "Screen"; 

  QVBoxLayout * layout = nullptr;
  InfoBarReqHdl * info_bar_handler = nullptr;
  ComponentReqHdl * main_content_handler = nullptr;
  UserTouchHandler * touch_handler = nullptr;


  void setup_base_ui();
  void updateWidgets();
  int uds_init(int * const event_fd, int * const server_fd);
  ComponentReqHdl * ensure_info_bar_handler(); 
  ComponentReqHdl * ensure_main_content_handler(
    bkk_screen_component_id_t component_id);

  static void * receive_thread_func(void * ctx);
  static int alive_check(void * ctx);
  static int screen_pwr_off_callback(void * ctx);
  static void screen_pwr_on_callback(ts_event_en event, void * ctx);
  bkk_screen_error_code_t dispatch_client_request(int client_fd);

  pthread_t receive_thread_fd = -1;

  int test_cnt = 0; 


  timer_thread_ctx_t alive_check_thread_ctx = {
    .config = {
      .timer_fd = -1,
      .cyclic_expiration_sec = 1,
      .cyclic_expiration_nsec = 0,
      .initial_expiration_sec = 1,
      .initial_expiration_nsec = 0,
    },
    .callback = alive_check,
    .arg = this,
    /*.stop_fd = -1,
    .is_running = false,
    .thread_created = false,
    .thread_joined = false,
    .thread = 0,
    .thread_id = 0,*/
  };


  timer_thread_ctx_t screen_pwr_off_timer_ctx = {
    .config = {
      .timer_fd = -1,
      .cyclic_expiration_sec = 10*60,
      .cyclic_expiration_nsec = 0,
      .initial_expiration_sec = 10*60,
      .initial_expiration_nsec = 0,
    },
    .callback = screen_pwr_off_callback,
    .arg = this,
  };

}; 


#endif // BKK_SCREEN_HPP