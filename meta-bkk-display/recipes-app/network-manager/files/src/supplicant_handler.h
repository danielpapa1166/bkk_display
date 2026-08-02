#ifndef SUPPLICANT_HANDLER_H
#define SUPPLICANT_HANDLER_H


void kill_all_supplicant_processes();
int start_supplicant(
  char * const wpa_cfg_path, 
  char * const wpa_interface_name);
int stop_supplicant(); 


#endif /* SUPPLICANT_HANDLER_H */