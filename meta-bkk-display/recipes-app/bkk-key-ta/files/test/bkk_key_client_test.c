#include "bkk_key_client.h"
#include <string.h>
#include <stdio.h>

static void fetch_and_print_error(void) {
  uint32_t error_status = 0U;
  uint32_t last_tee_error = 0U;

  const int fetch_res = bkk_key_fetch_error_status(&error_status, &last_tee_error);
  if (fetch_res != 0) {
    printf("Failed to fetch error status, error code: %d\n", fetch_res);
    return;
  }

  printf("Error status: %08X, Last TEE error: %08X\n", error_status, last_tee_error);
}


int main() {
  
  // --------------------------------------------------------------------------
  // test simple command
  // --------------------------------------------------------------------------

  /*printf("Starting BKK Key Client Test\n");
  const int test_res = bkk_key_test();
  if (test_res != 0) {
    printf("BKK Key Test failed, error code: %d\n", test_res);
    return test_res;
  }*/

  // --------------------------------------------------------------------------
  // test echo command
  // --------------------------------------------------------------------------

  char * echo_msg = "Hello, TEE!";
  char echo_response[256];
  size_t echo_response_len = sizeof(echo_response);

  const int echo_res = bkk_key_echo(
    echo_msg, strlen(echo_msg), 
    echo_response, &echo_response_len);

  if (echo_res != 0) {
    printf("Failed to echo message, error code: %d\n", echo_res);
    return echo_res;
  }

  printf("Echoed message: %s\n", echo_response);

  // --------------------------------------------------------------------------
  // test key write 
  // --------------------------------------------------------------------------

  const char * test_key = "test_key";
  size_t test_key_len = strlen(test_key);

  printf("Storing key: %s\n", test_key);
  const int store_res = bkk_key_store(test_key, test_key_len);
  if (store_res != 0) {
    printf("Failed to store key, error code: %d\n", store_res);
    fetch_and_print_error();
    return store_res;
  }

  // --------------------------------------------------------------------------
  // test key read
  // --------------------------------------------------------------------------

  char retrieved_key[256];
  size_t retrieved_key_len = sizeof(retrieved_key);

  const int retrieve_res = bkk_key_get(retrieved_key, &retrieved_key_len);
  if (retrieve_res != 0) {
    printf("Failed to retrieve key, error code: %d\n", retrieve_res);
    return retrieve_res;
  }

  printf("Retrieved key: %s\n", retrieved_key);

}
