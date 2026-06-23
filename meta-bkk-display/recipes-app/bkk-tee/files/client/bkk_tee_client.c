#include "bkk_tee_client.h"
#include "bkk_tee_common_defs.h"
#include <rbuflogd/logger.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <tee_client_api.h>

#define BKK_KEY_OBJ_ID "bkk_api_key"
#define BKK_KEY_OBJ_ID_LEN (sizeof(BKK_KEY_OBJ_ID)) 


typedef struct {
  TEEC_Context ctx;
  TEEC_Session sess;
} tee_client_ctx;

static const TEEC_UUID bkk_key_ta_uuid = {
  0x8f6f7b8a, 0x21a4, 0x4de8,
  { 0x9b, 0x8d, 0x7c, 0x0b, 0x96, 0x64, 0x8c, 0x19 }
};

static uint8_t logger_inited = 0; 
static const char * logger_name = "TEE_Clnt"; 
static const char * log_cat_set = "key_Set"; 
static const char * log_cat_get = "key_Get"; 
static const char * log_cat_test = "key_Test";
static const char * log_cat_echo = "key_Echo";

static void init_log(void) {
  if(logger_inited == 0) {
    const int ret = rbuflogd_logger_init(logger_name); 
    logger_inited = 1; 
  }
}

static int prepare_tee_session(tee_client_ctx *ctx)
{
  static const char * log_cat = "prep_ctx";
  char msg[100];
	uint32_t origin;
	TEEC_Result res;

	res = TEEC_InitializeContext(NULL, &ctx->ctx);
	if (res != TEEC_SUCCESS) {
    snprintf(msg, sizeof(msg), "TEEC_InitializeContext failed with code: %08X", res);
    log_error(log_cat, msg);
    return -1;
  }

	res = TEEC_OpenSession(
    &ctx->ctx, 
    &ctx->sess, 
    &bkk_key_ta_uuid,
    TEEC_LOGIN_PUBLIC, 
    NULL, 
    NULL, 
    &origin);

	if (res != TEEC_SUCCESS) {
    snprintf(msg, sizeof(msg), "TEEC_OpenSession failed with code: %08X, origin: %08X", res, origin);
    log_error(log_cat, msg);
    return -2;
  }

  return 0; 
}


static void close_tee_session(tee_client_ctx *ctx)
{
  TEEC_CloseSession(&ctx->sess);
  TEEC_FinalizeContext(&ctx->ctx);
}



int bkk_key_test(void) {
  init_log(); 

  log_info(log_cat_test, "Testing TEE communication with bkk_key_ta");

  char msg[100];

  TEEC_Context ctx;
  TEEC_Session sess;
  TEEC_Result res;
  uint32_t err_origin = 0U;


  tee_client_ctx client_ctx;
  res = prepare_tee_session(&client_ctx);
  if (res != 0) {
    snprintf(msg, sizeof(msg), "Failed to prepare TEE session, error code: %d", res);
    log_error(log_cat_test, msg);
    return res;
  }

  snprintf(msg, sizeof(msg), "Invoking command to test");
  log_info(log_cat_test, msg);

  res = TEEC_InvokeCommand(&client_ctx.sess, BKK_TEE_TEST_CMD, NULL, &err_origin);

  close_tee_session(&client_ctx);

  if(res != TEEC_SUCCESS) {
    snprintf(msg, sizeof(msg), "Failed to invoke command: %08X, error origin: %08X", res, err_origin);
    log_error(log_cat_test, msg);  
    return -4; 
  }


  log_info(log_cat_test, "Test command executed successfully in the TEE"); 

  return 0; 
}

int bkk_key_echo(const void *in, size_t in_len, void *out, size_t *out_len)
{
  init_log(); 

  char msg[100];
  snprintf(msg, sizeof(msg), "Echoing data of length %zu in the TEE", in_len);
  log_info(log_cat_echo, msg);

  TEEC_Context ctx;
  TEEC_Session sess;
  TEEC_Operation op;
  TEEC_Result res;
  uint32_t err_origin = 0U;

  if (!in || in_len == 0U || !out || !out_len || *out_len == 0U) {
    log_error(log_cat_echo, "Invalid argument"); 
    return -1;
  }

  res = TEEC_InitializeContext(NULL, &ctx);
  if (res != TEEC_SUCCESS) {
    char msg[100]; 
    snprintf(msg, sizeof(msg), "Failed to initialize context: %08X", res);
    log_error(log_cat_echo, msg); 
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
    log_error(log_cat_echo, msg);
    TEEC_FinalizeContext(&ctx);
    return -3;
  }

  memset(&op, 0, sizeof(op));
  op.paramTypes = TEEC_PARAM_TYPES(
    TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT,
    TEEC_NONE, TEEC_NONE);

  op.params[0].tmpref.buffer = (void *)in;
  op.params[0].tmpref.size = in_len;

  op.params[1].tmpref.buffer = out;
  op.params[1].tmpref.size = *out_len;

  snprintf(msg, sizeof(msg), "Invoking command to echo data in the TEE, paramTypes: %08X", op.paramTypes);
  log_info(log_cat_echo, msg);

  res = TEEC_InvokeCommand(&sess, BKK_TEE_ECHO_CMD, &op, &err_origin);

  TEEC_CloseSession(&sess);
  TEEC_FinalizeContext(&ctx);

  if(res != TEEC_SUCCESS) {
    char msg[100];
    snprintf(msg, sizeof(msg), "Failed to invoke command: %08X, error origin: %08X", res, err_origin);
    log_error(log_cat_echo, msg);
    return -4;
  }

  *out_len = op.params[1].tmpref.size;
  log_info(log_cat_echo, "Echo command executed successfully in the TEE"); 

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
    TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT,
    TEEC_NONE, TEEC_NONE);
  
  op.params[0].tmpref.buffer = (void *)BKK_KEY_OBJ_ID;
  op.params[0].tmpref.size = BKK_KEY_OBJ_ID_LEN;
  op.params[1].tmpref.buffer = (void *)key;
  op.params[1].tmpref.size = key_len;

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

  log_info(log_cat_get, "Fetching api key from the TEE, init context");

  res = TEEC_InitializeContext(NULL, &ctx);
  if (res != TEEC_SUCCESS) {
    snprintf(msg, sizeof(msg), "Failed to initialize context: %08X", res);
    log_error(log_cat_get, msg);
    return -2;
  }

  log_info(log_cat_get, "Opening session with the TEE");

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
    TEEC_MEMREF_TEMP_INPUT,
    TEEC_MEMREF_TEMP_OUTPUT,
    TEEC_NONE,
    TEEC_NONE);

  op.params[0].tmpref.buffer = BKK_KEY_OBJ_ID; 
  op.params[0].tmpref.size = BKK_KEY_OBJ_ID_LEN;
  op.params[1].tmpref.buffer = buf;
  op.params[1].tmpref.size = *buf_len;

  log_info(log_cat_get, "Invoking command to fetch api key from the TEE");

  res = TEEC_InvokeCommand(&sess, BKK_KEY_CMD_GET, &op, &err_origin);
  //*buf_len = op.params[1].tmpref.size;

  log_info(log_cat_get, "Closing session and finalizing context");

  TEEC_CloseSession(&sess);
  TEEC_FinalizeContext(&ctx);

  if(res != TEEC_SUCCESS) {
    snprintf(msg, sizeof(msg), "Failed to invoke command: %08X, error origin: %08X", res, err_origin);
    log_error(log_cat_get, msg);
    return -4;
  }

  log_info(log_cat_get, "Successfully fetched api key from the TEE");

  return 0;
}
