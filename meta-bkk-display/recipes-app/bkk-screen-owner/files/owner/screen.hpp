#ifndef BKK_SCREEN_HPP
#define BKK_SCREEN_HPP

#include <QWidget>
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
  screen_error_t expose_screen_components(void); 

private: 

  QVBoxLayout * layout = nullptr;
  QWidget * infoBar = nullptr;
  QWidget * contentWidget = nullptr;
  void setup_base_ui();

  int shmem_fd = -1;
  bkk_screen_component_list_t * component_list = nullptr;

}; 


#endif // BKK_SCREEN_HPP