#ifndef CONTENT_STATE_MACHINE_HPP
#define CONTENT_STATE_MACHINE_HPP


#define DBUS_PEER_NAME "bkk-screen-main-content"

namespace content_sm {
  
typedef enum content_state {
  INIT = 0,
  ACCESS_POINT_MODE,
  CONFIG_API_MODE, 
  NORMAL_DISPLAY_MODE,
  UNKNOWN,
} content_state_t;

int init();
int ping_timer_callback(void * arg); 
int reinit(); 
int exec_fun();
int switch_state(content_state_t new_state);
int exit();

} // namespace content_sm

#endif // CONTENT_STATE_MACHINE_HPP
