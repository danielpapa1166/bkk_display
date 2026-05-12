#ifndef AM_LOGGER_H
#define AM_LOGGER_H


int init_logger(void); 
void cleanup_logger(void);
void rename_logger(const char *new_name, int length);
int log_debug(const char * const category, const char * const message);
int log_info(const char * const category, const char * const message);
int log_warning(const char * const category, const char * const message);
int log_error(const char * const category, const char * const message);


#endif /* AM_LOGGER_H */