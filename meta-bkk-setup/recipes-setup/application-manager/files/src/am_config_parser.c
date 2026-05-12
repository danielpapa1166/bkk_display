#include "am_config_parser.h"
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ----------------------------------------------------------------------------
// --- helper functions ---
// ----------------------------------------------------------------------------


// this function reads the entire contents of the config file 
// and stores it into a newly allocated string
static parse_status_t load_config_file(const char * config_path, char ** data_out) {
  FILE * fid = fopen(config_path, "r");
  if(!fid) {
    return PARSE_ERR_CFG_FILE_NOT_FOUND; 
  }

  char * file_contents = NULL;
  fseek(fid, 0, SEEK_END);
  long file_size = ftell(fid);
  
  file_contents = (char *)malloc(file_size + 1);
  if (!file_contents) {
    fclose(fid);
    return PARSE_ERR_CFG_FILE_INVALID;
  }
    
  fseek(fid, 0, SEEK_SET);
  fread(file_contents, 1, file_size, fid);

  file_contents[file_size] = '\0';
  fclose(fid); 
  *data_out = file_contents;
  return PARSE_OK;
}


// this function parses a cJSON array of strings into 
// a newly allocated array of C strings
static parse_status_t parse_array_of_strings(cJSON * json_array, 
    char *** out_strings, int * out_num_strings) {
  if (!cJSON_IsArray(json_array)) {
    return PARSE_ERR_JSON_INVALID;
  }
  int num_strings = cJSON_GetArraySize(json_array);
  char ** strings = (char **)malloc(sizeof(char *) * num_strings);
  for (int i = 0; i < num_strings; i++) {
    cJSON * item = cJSON_GetArrayItem(json_array, i);
    if (!cJSON_IsString(item)) {
      return PARSE_ERR_JSON_INVALID;
    }
    strings[i] = strdup(item->valuestring);
  }
  *out_strings = strings;
  *out_num_strings = num_strings;
  return PARSE_OK;
}

// this function parses a single cJSON object representing an app config into 
// an app_config_t struct
static parse_status_t parse_json_element(
    cJSON * element, app_config_t * app_out) {
  
  cJSON * name = cJSON_GetObjectItem(element, "name");
  if (!cJSON_IsString(name)) {
    return PARSE_ERR_JSON_INVALID;
  }
  app_out->name = strdup(name->valuestring);

  cJSON * binary = cJSON_GetObjectItem(element, "binary");
  if (!cJSON_IsString(binary)) {
    return PARSE_ERR_JSON_INVALID;
  }
  app_out->binary = strdup(binary->valuestring);


  parse_status_t status = parse_array_of_strings(
    cJSON_GetObjectItem(element, "args"), 
    &app_out->args, &app_out->num_args);
  
  if(status != PARSE_OK) {
    return status;
  }

  status = parse_array_of_strings(
    cJSON_GetObjectItem(element, "phases"), 
    &app_out->phases, &app_out->num_phases);
  
  if(status != PARSE_OK) {
    return status;
  }

  cJSON * after = cJSON_GetObjectItem(element, "after");
  if (cJSON_IsString(after)) {
    app_out->after = strdup(after->valuestring);
  } else {
    app_out->after = NULL;
  }

  return PARSE_OK;
}


// this function parses the entire config JSON string into 
// an array of app_config_t structs
static parse_status_t parse_json_config(
    const char * json_str, app_config_list_t * config_list_out) {

  cJSON * json = cJSON_Parse(json_str);
  if (!json) {
    return PARSE_ERR_JSON_INVALID;
  }

  if (!cJSON_IsArray(json)) {
    cJSON_Delete(json);
    return PARSE_ERR_JSON_INVALID;
  }

  int num_apps = cJSON_GetArraySize(json);
  app_config_t * apps = (app_config_t *)malloc(sizeof(app_config_t) * num_apps);

  for (int i = 0; i < num_apps; i++) {
    cJSON * element = cJSON_GetArrayItem(json, i);
    parse_status_t status = parse_json_element(element, &apps[i]);
    if (status != PARSE_OK) {
      cJSON_Delete(json);
      return status;
    }
  }

  config_list_out->apps = apps;
  config_list_out->num_apps = num_apps;

  cJSON_Delete(json);

  return PARSE_OK;
}

// ----------------------------------------------------------------------------
// --- main parsing functions ---
// ----------------------------------------------------------------------------

// this function parses the command line arguments to find the config file path
parse_status_t parse_cli(int argc, char ** argv, char ** config_path_out) {

  for (int i = 1; i < argc; i++) {
    const char * arg = argv[i];
    if (strcmp(arg, "--config") == 0) {
      if (i + 1 >= argc) {
        return PARSE_ERR_CLI;
      }
      i++;
      *config_path_out = argv[i];
      return PARSE_OK;
    }
  }

  return PARSE_ERR_CLI;
}


// this function parses the config file at the given path into 
// an array of app configs
parse_status_t parse_config(const char * config_path, 
    app_config_list_t * config_list_out) {

  char * file_contents = NULL;
  parse_status_t status = load_config_file(
    config_path, &file_contents);
  
  if (status != PARSE_OK) {
    return status;
  }

  status = parse_json_config(
    file_contents, config_list_out);
    
  free(file_contents);
  return status;
}