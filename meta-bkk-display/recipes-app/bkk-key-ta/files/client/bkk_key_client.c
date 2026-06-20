#include "bkk_key_client.h"
#include <rbuflogd/logger.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <tee_client_api.h>

#define BKK_KEY_CMD_STORE 0U
#define BKK_KEY_CMD_GET   1U
#define BKK_KEY_TEST_CMD  2U

static const TEEC_UUID bkk_key_ta_uuid = {
  0x8f6f7b8a, 0x21a4, 0x4de8,
  { 0x9b, 0x8d, 0x7c, 0x0b, 0x96, 0x64, 0x8c, 0x19 }
};

static uint8_t logger_inited = 0; 
static const char * logger_name = "TEE_Clnt"; 
static const char * log_cat_set = "key_Set"; 
static const char * log_cat_get = "key_Get"; 
static const char * log_cat_test = "key_Tst";

static void init_log(void) {
  if(logger_inited == 0) {
    const int ret = rbuflogd_logger_init(logger_name); 
    logger_inited = 1; 
  }
}


int bkk_key_test(void) {
  init_log(); 

  log_info(log_cat_test, "Testing TEE communication with bkk_key_ta");

  char msg[100];

  TEEC_Context ctx;
  TEEC_Session sess;
  TEEC_Result res;
  uint32_t err_origin = 0U;

  res = TEEC_InitializeContext(NULL, &ctx);
  if (res != TEEC_SUCCESS) {
    char msg[100]; 
    snprintf(msg, sizeof(msg), "Failed to initialize context: %08X", res);
    log_error(log_cat_test, msg); 
    return -2;
  }

  res = TEEC_OpenSession(
    &ctx,                 // TEEC_Context* context 
    &sess,                // TEEC_Session* session 
    &bkk_key_ta_uuid,     // const TEEC_UUID* destination 
    TEEC_LOGIN_USER, // TEEC_LOGIN_PUBLIC,    // uint32_t connectionMethod       
    NULL,                 // const void* connectionData 
    NULL,                 // TEEC_Operation* operation 
    &err_origin           // uint32_t* returnOrigin 
  );
  if (res != TEEC_SUCCESS) {
    char msg[100]; 
    snprintf(msg, sizeof(msg), "Failed to open session: %08X, err origin: %08X", res, err_origin); 
    log_error(log_cat_test, msg); 
    TEEC_FinalizeContext(&ctx);
    return -3;
  }


  snprintf(msg, sizeof(msg), "Invoking command to test");
  log_info(log_cat_test, msg);

  res = TEEC_InvokeCommand(&sess, BKK_KEY_TEST_CMD, NULL, &err_origin);

  TEEC_CloseSession(&sess);
  TEEC_FinalizeContext(&ctx);

  if(res != TEEC_SUCCESS) {
    char msg[100]; 
    snprintf(msg, sizeof(msg), "Failed to invoke command: %08X, error origin: %08X", res, err_origin);
    log_error(log_cat_test, msg);  
    return -4; 
  }


  log_info(log_cat_test, "Test command executed successfully in the TEE"); 

  return 0; 
}

int bkk_key_store(const void *key, size_t key_len)
{
  init_log(); 

  char msg[100];
  snprintf(msg, sizeof(msg), "Storing api key %s of length %zu in the TEE", (const char *)key, key_len);
  log_info(log_cat_set, msg);

  TEEC_Context ctx;
  TEEC_Session sess;
  TEEC_Operation op;
  TEEC_Result res;
  uint32_t err_origin = 0U;

  if (!key || key_len == 0U) {
    log_error(log_cat_set, "Invalid argument"); 
    return -1;
  }

  res = TEEC_InitializeContext(NULL, &ctx);
  if (res != TEEC_SUCCESS) {
    char msg[100]; 
    snprintf(msg, sizeof(msg), "Failed to initialize context: %08X", res);
    log_error(log_cat_set, msg); 
    return -2;
  }

  res = TEEC_OpenSession(
    &ctx,                 // TEEC_Context* context 
    &sess,                // TEEC_Session* session 
    &bkk_key_ta_uuid,     // const TEEC_UUID* destination 
    TEEC_LOGIN_USER, // TEEC_LOGIN_PUBLIC,    // uint32_t connectionMethod       
    NULL,                 // const void* connectionData 
    NULL,                 // TEEC_Operation* operation 
    &err_origin           // uint32_t* returnOrigin 
  );
  if (res != TEEC_SUCCESS) {
    char msg[100]; 
    snprintf(msg, sizeof(msg), "Failed to open session: %08X, err origin: %08X", res, err_origin); 
    log_error(log_cat_set, msg); 
    TEEC_FinalizeContext(&ctx);
    return -3;
  }

  memset(&op, 0, sizeof(op));
  op.paramTypes = TEEC_PARAM_TYPES(
    TEEC_MEMREF_TEMP_INPUT, TEEC_NONE,
    TEEC_NONE, TEEC_NONE);
    
  op.params[0].tmpref.buffer = (void *)key;
  op.params[0].tmpref.size = key_len;

  snprintf(msg, sizeof(msg), "Invoking command to store api key in the TEE, paramTypes: %08X", op.paramTypes);
  log_info(log_cat_set, msg);

  res = TEEC_InvokeCommand(&sess, BKK_KEY_CMD_STORE, &op, &err_origin);

  TEEC_CloseSession(&sess);
  TEEC_FinalizeContext(&ctx);

  if(res != TEEC_SUCCESS) {
    char msg[100]; 
    snprintf(msg, sizeof(msg), "Failed to invoke command: %08X, error origin: %08X", res, err_origin);
    log_error(log_cat_set, msg);  
    return -4; 
  }


  log_info(log_cat_set, "Successfully stored api key in the TEE"); 

  return 0;
}

int bkk_key_get(void *buf, size_t *buf_len)
{
  char msg[100];
  init_log(); 
  TEEC_Context ctx;
  TEEC_Session sess;
  TEEC_Operation op;
  TEEC_Result res;
  uint32_t err_origin = 0U;

  if (!buf || !buf_len || *buf_len == 0U) {
    log_error(log_cat_get, "Invalid argument");
    return -1;
  }

  res = TEEC_InitializeContext(NULL, &ctx);
  if (res != TEEC_SUCCESS) {
    snprintf(msg, sizeof(msg), "Failed to initialize context: %08X", res);
    log_error(log_cat_get, msg);
    return -2;
  }

  res = TEEC_OpenSession(&ctx, &sess, &bkk_key_ta_uuid, TEEC_LOGIN_PUBLIC,
               NULL, NULL, &err_origin);
  if (res != TEEC_SUCCESS) {
    snprintf(msg, sizeof(msg), "Failed to open session: %08X, err origin: %08X", res, err_origin);
    log_error(log_cat_get, msg);
    TEEC_FinalizeContext(&ctx);
    return -3;
  }

  memset(&op, 0, sizeof(op));
  op.paramTypes = TEEC_PARAM_TYPES(
    TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE,
    TEEC_NONE, TEEC_NONE);

  op.params[0].tmpref.buffer = buf;
  op.params[0].tmpref.size = *buf_len;

  res = TEEC_InvokeCommand(&sess, BKK_KEY_CMD_GET, &op, &err_origin);
  *buf_len = op.params[0].tmpref.size;

  TEEC_CloseSession(&sess);
  TEEC_FinalizeContext(&ctx);

  if(res != TEEC_SUCCESS) {
    snprintf(msg, sizeof(msg), "Failed to invoke command: %08X, error origin: %08X", res, err_origin);
    log_error(log_cat_get, msg);
    return -4;
  }

  return 0;
}
