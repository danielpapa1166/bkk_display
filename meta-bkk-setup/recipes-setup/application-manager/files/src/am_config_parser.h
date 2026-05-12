#ifndef AM_CONFIG_PARSER_H
#define AM_CONFIG_PARSER_H

#include "am_types.h"

typedef enum {
  PARSE_OK,
  PARSE_ERR_CLI,
  PARSE_ERR_CFG_FILE_NOT_FOUND,
  PARSE_ERR_CFG_FILE_INVALID,
  PARSE_ERR_JSON_INVALID
} parse_status_t;

parse_status_t parse_cli(int argc, char ** argv, char ** config_path_out);
parse_status_t parse_config(const char * config_path, 
  app_config_list_t * config_list_out);


#endif // AM_CONFIG_PARSER_H