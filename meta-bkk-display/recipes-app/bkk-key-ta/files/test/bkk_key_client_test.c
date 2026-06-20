#include "bkk_key_client.h"
#include <string.h>
#include <stdio.h>

int main() {
  

  printf("Starting BKK Key Client Test\n");
  const int test_res = bkk_key_test();
  if (test_res != 0) {
    printf("BKK Key Test failed, error code: %d\n", test_res);
    return test_res;
  }

  const char * test_key = "test_key";
  size_t test_key_len = strlen(test_key);

  printf("Storing key: %s\n", test_key);
  const int store_res = bkk_key_store(test_key, test_key_len);
  if (store_res != 0) {
    printf("Failed to store key, error code: %d\n", store_res);
    return store_res;
  }

  char retrieved_key[256];
  size_t retrieved_key_len = sizeof(retrieved_key);

  const int retrieve_res = bkk_key_get(retrieved_key, &retrieved_key_len);
  if (retrieve_res != 0) {
    printf("Failed to retrieve key, error code: %d\n", retrieve_res);
    return retrieve_res;
  }

  printf("Retrieved key: %s\n", retrieved_key);

}
