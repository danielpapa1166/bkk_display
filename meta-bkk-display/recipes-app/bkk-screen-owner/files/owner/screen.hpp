#ifndef BKK_SCREEN_HPP
#define BKK_SCREEN_HPP

#include <QWidget>
#include <QVBoxLayout>
#include "bkk_screen_common_priv_defs.hpp"

typedef enum {
  BKK_SCREEN_ERROR_NONE, 
  BKK_SCREEN_ERROR_OTHER
} screen_error_t; 


class BkkScreen : public QWidget
{
  Q_OBJECT

public:
  explicit BkkScreen(QWidget *parent = nullptr);
  ~BkkScreen();
  int start_receive_thread(); 

private: 

  QVBoxLayout * layout = nullptr;
  QWidget * infoBar = nullptr;
  QWidget * contentWidget = nullptr;
  void setup_base_ui();
  int uds_init(int * const event_fd, int * const server_fd);

  static void * receive_thread_func(void * ctx);
  int handle_client_request(int client_fd);

  int receive_thread_fd = -1;
}; 


#endif // BKK_SCREEN_HPP