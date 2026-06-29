#include "bkk_tee_client.h"
#include "bkk_tee_common_defs.h"
#include <rbuflogd/logger.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <tee_client_api.h>

// ----------------------------------------------------------------------------
// internal data structures and constants
// ----------------------------------------------------------------------------

typedef struct {
  TEEC_Context ctx;
  TEEC_Session sess;
} tee_client_ctx;

static const TEEC_UUID bkk_tee_ta_uuid = {
  0x8f6f7b8a, 0x21a4, 0x4de8,
  { 0x9b, 0x8d, 0x7c, 0x0b, 0x96, 0x64, 0x8c, 0x19 }
};

static uint8_t logger_inited = 0; 
static const char * logger_name = "TEE_Clnt"; 
static const char * log_cat_set = "key_Set"; 
static const char * log_cat_get = "key_Get"; 
static const char * log_cat_test = "key_Test";
static const char * log_cat_echo = "key_Echo";

// ----------------------------------------------------------------------------
// internal helper functions
// ----------------------------------------------------------------------------

const char *bkk_tee_get_object_type_name(tee_object_type_t obj_type) {
  switch (obj_type) {
    case tee_object_type_api_key:
      return "API Key";
    case tee_object_type_wifi_pw:
      return "WiFi Password";
    default:
      return "Unknown";  
  }
}

static void init_log(void) {
  if(logger_inited == 0) {
    const int ret = rbuflogd_logger_init(logger_name); 
    logger_inited = 1; 
  }
}

static bkk_tee_client_status_t prepare_tee_session(tee_client_ctx *ctx) {
  static const char * log_cat = "prep_ctx";
  char msg[100];
	uint32_t origin;
	TEEC_Result res;

	res = TEEC_InitializeContext(NULL, &ctx->ctx);
	if (res != TEEC_SUCCESS) {
    snprintf(msg, sizeof(msg), 
      "TEEC_InitializeContext failed with code: %08X", res);
    log_error(log_cat, msg);
    return bkk_tee_client_err_session_failed;
  }

	res = TEEC_OpenSession(
    &ctx->ctx, 
    &ctx->sess, 
    &bkk_tee_ta_uuid,
    TEEC_LOGIN_PUBLIC, 
    NULL, 
    NULL, 
    &origin);

	if (res != TEEC_SUCCESS) {
    snprintf(msg, sizeof(msg), 
      "TEEC_OpenSession failed with code: %08X, origin: %08X", res, origin);
    log_error(log_cat, msg);
    return bkk_tee_client_err_session_failed;
  }

  return bkk_tee_client_err_none;
}


static void close_tee_session(tee_client_ctx *ctx) {
  TEEC_CloseSession(&ctx->sess);
  TEEC_FinalizeContext(&ctx->ctx);
}


// ----------------------------------------------------------------------------
// TEE Client API functions
// ----------------------------------------------------------------------------

bkk_tee_client_status_t bkk_tee_test(void) {
  init_log(); 

  char msg[100];
  uint32_t err_origin = 0U;


  tee_client_ctx client_ctx;
  const bkk_tee_client_status_t sess_res = prepare_tee_session(&client_ctx);
  if (sess_res != bkk_tee_client_err_none) {
    snprintf(msg, sizeof(msg), 
      "Failed to prepare TEE session, error code: %d", sess_res);
    log_error(log_cat_test, msg);
    return sess_res;
  }

  const TEEC_Result teec_res = TEEC_InvokeCommand(
    &client_ctx.sess, 
    BKK_TEE_TEST_CMD, 
    NULL, 
    &err_origin);

  close_tee_session(&client_ctx);

  if(teec_res != TEEC_SUCCESS) {
    snprintf(msg, sizeof(msg), 
      "Failed to invoke command: %08X, error origin: %08X", 
      teec_res, err_origin);
    log_error(log_cat_test, msg);  
    return bkk_tee_client_err_invoke_failed; 
  }

  log_info(log_cat_test, "Test command executed successfully in the TEE"); 
  return bkk_tee_client_err_none; 
}

bkk_tee_client_status_t bkk_tee_echo(
    const void *in, size_t in_len, 
    void *out, size_t *out_len) {
  
  init_log(); 

  char msg[100];

  TEEC_Operation op;
  tee_client_ctx client_ctx;
  uint32_t err_origin = 0U;

  if (!in || in_len == 0U || !out || !out_len || *out_len == 0U) {
    log_error(log_cat_echo, "Invalid argument"); 
    return bkk_tee_client_err_invalid_arg;
  }

  const bkk_tee_client_status_t sess_res = prepare_tee_session(&client_ctx);
  if (sess_res != bkk_tee_client_err_none) {
    snprintf(msg, sizeof(msg), 
      "Failed to prepare TEE session, error code: %d", sess_res);
    log_error(log_cat_echo, msg);
    return sess_res;
  }

  memset(&op, 0, sizeof(op));
  op.paramTypes = TEEC_PARAM_TYPES(
    TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT,
    TEEC_NONE, TEEC_NONE);

  op.params[0].tmpref.buffer = (void *)in;
  op.params[0].tmpref.size = in_len;

  op.params[1].tmpref.buffer = out;
  op.params[1].tmpref.size = *out_len;

  const TEEC_Result teec_res = TEEC_InvokeCommand(
    &client_ctx.sess, 
    BKK_TEE_ECHO_CMD, 
    &op, 
    &err_origin);

  close_tee_session(&client_ctx);

  if(teec_res != TEEC_SUCCESS) {
    snprintf(msg, sizeof(msg), 
      "Failed to invoke command: %08X, error origin: %08X", 
      teec_res, err_origin);
    log_error(log_cat_echo, msg);
    return bkk_tee_client_err_invoke_failed;
  }

  *out_len = op.params[1].tmpref.size;
  log_info(log_cat_echo, "Echo command executed successfully in the TEE"); 

  return bkk_tee_client_err_none;
}


bkk_tee_client_status_t bkk_tee_store(
    tee_object_type_t obj_type, 
    const void *buf, size_t buf_len) {

  init_log(); 

  if( obj_type != tee_object_type_api_key 
      && obj_type != tee_object_type_wifi_pw) {
    log_error(log_cat_set, "Invalid object type"); 
    return bkk_tee_client_err_invalid_arg;
  }

  char msg[100];
  tee_client_ctx client_ctx;
  TEEC_Operation op;
  uint32_t err_origin = 0U;

  if (!buf || buf_len == 0U) {
    log_error(log_cat_set, "Invalid argument"); 
    return bkk_tee_client_err_invalid_arg;
  }

  const bkk_tee_client_status_t sess_res = prepare_tee_session(&client_ctx);
  if (sess_res != bkk_tee_client_err_none) {
    snprintf(msg, sizeof(msg), 
      "Failed to prepare TEE session, error code: %d", sess_res);
    log_error(log_cat_set, msg);
    return sess_res;
  }

  memset(&op, 0, sizeof(op));
  op.paramTypes = TEEC_PARAM_TYPES(
    TEEC_MEMREF_TEMP_INPUT, 
    TEEC_MEMREF_TEMP_OUTPUT, // debug output
    TEEC_NONE, 
    TEEC_NONE);
  
  char debug_buf[BKK_TEE_MAX_OBJ_ID_LEN];

  op.params[0].tmpref.buffer = (void *)buf;
  op.params[0].tmpref.size = buf_len;
  op.params[1].tmpref.buffer = debug_buf;
  op.params[1].tmpref.size = sizeof(debug_buf);
  

  uint32_t cmd  
    = (obj_type == tee_object_type_api_key) 
    ? BKK_KEY_CMD_STORE : BKK_WIFI_PW_CMD_STORE; 

  const TEEC_Result teec_res = TEEC_InvokeCommand(
    &client_ctx.sess, 
    cmd, 
    &op, 
    &err_origin);

  close_tee_session(&client_ctx);

  if(teec_res != TEEC_SUCCESS) {
    snprintf(msg, sizeof(msg), 
      "Failed to invoke command: %08X, error origin: %08X", teec_res, err_origin);
    log_error(log_cat_set, msg);  
    return bkk_tee_client_err_invoke_failed;
  }

  return bkk_tee_client_err_none;
}

bkk_tee_client_status_t bkk_tee_get(tee_object_type_t obj_type, void *buf, size_t *buf_len) {
  char msg[100];
  init_log(); 

  if( obj_type != tee_object_type_api_key 
      && obj_type != tee_object_type_wifi_pw) {
    log_error(log_cat_get, "Invalid object type"); 
    return bkk_tee_client_err_invalid_arg;
  }

  tee_client_ctx client_ctx;
  TEEC_Operation op;
  uint32_t err_origin = 0U;

  if (!buf || !buf_len || *buf_len == 0U) {
    log_error(log_cat_get, "Invalid argument");
    return bkk_tee_client_err_invalid_arg;
  }

  const bkk_tee_client_status_t sess_res = prepare_tee_session(&client_ctx);
  if (sess_res != bkk_tee_client_err_none) {
    snprintf(msg, sizeof(msg), 
      "Failed to prepare TEE session, error code: %d", sess_res);
    log_error(log_cat_get, msg);
    return sess_res;
  }

  memset(&op, 0, sizeof(op));
  op.paramTypes = TEEC_PARAM_TYPES(
    TEEC_MEMREF_TEMP_OUTPUT,
    TEEC_MEMREF_TEMP_OUTPUT, // debug output
    TEEC_NONE,
    TEEC_NONE);

  char debug_buf[BKK_TEE_MAX_OBJ_ID_LEN];

  op.params[0].tmpref.buffer = buf;
  op.params[0].tmpref.size = *buf_len;
  op.params[1].tmpref.buffer = debug_buf;
  op.params[1].tmpref.size = sizeof(debug_buf);

  uint32_t cmd  
    = (obj_type == tee_object_type_api_key) 
    ? BKK_KEY_CMD_GET : BKK_WIFI_PW_CMD_GET;
  const TEEC_Result teec_res = TEEC_InvokeCommand(
    &client_ctx.sess, 
    cmd, 
    &op, 
    &err_origin);

  *buf_len = op.params[0].tmpref.size;

  close_tee_session(&client_ctx);

  if(teec_res != TEEC_SUCCESS) {
    snprintf(msg, sizeof(msg), 
      "Failed to invoke command: %08X, error origin: %08X", teec_res, err_origin);
    log_error(log_cat_get, msg);
    return bkk_tee_client_err_invoke_failed;
  }

  return bkk_tee_client_err_none;
}
