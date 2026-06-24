#include "bkk_tee_client.h"
#include <string.h>
#include <stdio.h>


static void print_usage(const char *prog_name) {
  printf("Usage: %s [options]\n", prog_name);
  printf("Options:\n");
  printf("  -test                     Run a simple test command in the TEE\n");
  printf("  -echo <message>           Echo a message in the TEE\n");
  printf("  -store_key <key>          Store an API key in the TEE\n");
  printf("  -retrieve_key             Retrieve the stored API key from the TEE\n");
  printf("  -store_wifi_pw <password> Store a WiFi password in the TEE\n");
  printf("  -retrieve_wifi_pw         Retrieve the stored WiFi password from the TEE \n"); 
}

int main(int argc, char *argv[]) {

  if(argc < 2) {
    print_usage(argv[0]);
    return -1;
  }

  const char *option = argv[1];

  if(strcmp(option, "-test") == 0) {
    // ------------------------------------------------------------------------
    // test simple command
    // ------------------------------------------------------------------------
    const int test_res = bkk_tee_test();
    if(test_res != 0) {
      printf("BKK Key TEE Test failed, error code: %d\n", test_res);
    }
    else {
      printf("BKK Key TEE Test succeeded\n");
    }
    return test_res;

  } 
  else if(strcmp(option, "-echo") == 0) {
    // ------------------------------------------------------------------------
    // test echo command
    // ------------------------------------------------------------------------
    if(argc < 3) {
      printf("Error: Missing message for echo command\n");
      print_usage(argv[0]);
      return -1;
    }
    const char *message = argv[2];
    char echo_response[256];
    size_t echo_response_len = sizeof(echo_response);
    const int echo_res = bkk_tee_echo(
      message, 
      strlen(message), 
      echo_response, 
      &echo_response_len);

    if (echo_res != 0) {
      printf("Failed to echo message, error code: %d\n", echo_res);
      return echo_res;
    }
    printf("Echoed message: %s\n", echo_response);

  } 
  else if(strcmp(option, "-store_key") == 0) {
    // ------------------------------------------------------------------------
    // test key write
    // ------------------------------------------------------------------------
    if(argc < 3) {
      printf("Error: Missing key for store command\n");
      print_usage(argv[0]);
      return -1;
    }
    const char *key = argv[2];
    const int store_res = bkk_tee_store(
      tee_object_type_api_key, key, strlen(key));
    if (store_res != 0) {
      printf("Failed to store key, error code: %d\n", store_res);
      return store_res;
    }
    printf("Successfully stored key: %s\n", key);
  } 
  else if(strcmp(option, "-retrieve_key") == 0) {
    // ------------------------------------------------------------------------
    // test key read
    // ------------------------------------------------------------------------
    char retrieved_key[256];
    size_t retrieved_key_len = sizeof(retrieved_key);
    const int retrieve_res = bkk_tee_get(
      tee_object_type_api_key, retrieved_key, &retrieved_key_len);
    if (retrieve_res != 0) {
      printf("Failed to retrieve key, error code: %d\n", retrieve_res);
      return retrieve_res;
    }
    printf("Retrieved key: %s\n", retrieved_key);
  } 
  else if(strcmp(option, "-store_wifi_pw") == 0) {
    // ------------------------------------------------------------------------
    // test WiFi password write
    // ------------------------------------------------------------------------
    if(argc < 3) {
      printf("Error: Missing WiFi password for store command\n");
      print_usage(argv[0]);
      return -1;
    }
    const char *wifi_pw = argv[2];
    const int store_res = bkk_tee_store(
      tee_object_type_wifi_pw, wifi_pw, strlen(wifi_pw));
    if (store_res != 0) {
      printf("Failed to store WiFi password, error code: %d\n", store_res);
      return store_res;
    }
    printf("Successfully stored WiFi password: %s\n", wifi_pw);
  } 
  else if(strcmp(option, "-retrieve_wifi_pw") == 0) {
    // ------------------------------------------------------------------------
    // test WiFi password read
    // ------------------------------------------------------------------------
    char retrieved_wifi_pw[256];
    size_t retrieved_wifi_pw_len = sizeof(retrieved_wifi_pw);
    const int retrieve_res = bkk_tee_get(
      tee_object_type_wifi_pw, retrieved_wifi_pw, &retrieved_wifi_pw_len);
    if (retrieve_res != 0) {
      printf("Failed to retrieve WiFi password, error code: %d\n", retrieve_res);
      return retrieve_res;
    }
    printf("Retrieved WiFi password: %s\n", retrieved_wifi_pw);
  }
  else {
    printf("Error: Unknown option '%s'\n", option);
    print_usage(argv[0]);
    return -1;
  }
}
