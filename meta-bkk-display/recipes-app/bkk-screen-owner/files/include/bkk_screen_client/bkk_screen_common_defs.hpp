#ifndef BKK_SCREEN_COMMON_DEFS_HPP
#define BKK_SCREEN_COMMON_DEFS_HPP

typedef enum {
  BKK_SCREEN_ERROR_NONE, 
  BKK_SCREEN_ERROR_OTHER 
} bkk_screen_error_code_t;

typedef enum {
  BKK_SCREEN_COMPONENT_INFO_BAR = 0, 
  BKK_SCREEN_COMPONENT_MAX
} bkk_screen_component_id_t;

typedef struct {
  void * instance; 
  bkk_screen_component_id_t component_id;
  bool taken; 
} bkk_screen_component_t;


#endif // BKK_SCREEN_COMMON_DEFS_HPP