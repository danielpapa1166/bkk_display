#include "bkk_key_client.h"

#include <stdint.h>
#include <string.h>

#include <tee_client_api.h>

#define BKK_KEY_CMD_STORE 0U
#define BKK_KEY_CMD_GET   1U

static const TEEC_UUID bkk_key_ta_uuid = {
  0x8f6f7b8a, 0x21a4, 0x4de8,
  { 0x9b, 0x8d, 0x7c, 0x0b, 0x96, 0x64, 0x8c, 0x19 }
};

int bkk_key_store(const void *key, size_t key_len)
{
  TEEC_Context ctx;
  TEEC_Session sess;
  TEEC_Operation op;
  TEEC_Result res;
  uint32_t err_origin = 0U;

  if (!key || key_len == 0U) {
    return -1;
  }

  res = TEEC_InitializeContext(NULL, &ctx);
  if (res != TEEC_SUCCESS) {
    return -2;
  }

  res = TEEC_OpenSession(&ctx, &sess, &bkk_key_ta_uuid, TEEC_LOGIN_PUBLIC,
               NULL, NULL, &err_origin);
  if (res != TEEC_SUCCESS) {
    TEEC_FinalizeContext(&ctx);
    return -3;
  }

  memset(&op, 0, sizeof(op));
  op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_NONE,
                   TEEC_NONE, TEEC_NONE);
  op.params[0].tmpref.buffer = (void *)key;
  op.params[0].tmpref.size = key_len;

  res = TEEC_InvokeCommand(&sess, BKK_KEY_CMD_STORE, &op, &err_origin);

  TEEC_CloseSession(&sess);
  TEEC_FinalizeContext(&ctx);

  return (res == TEEC_SUCCESS) ? 0 : -4;
}

int bkk_key_get(void *buf, size_t *buf_len)
{
  TEEC_Context ctx;
  TEEC_Session sess;
  TEEC_Operation op;
  TEEC_Result res;
  uint32_t err_origin = 0U;

  if (!buf || !buf_len || *buf_len == 0U) {
    return -1;
  }

  res = TEEC_InitializeContext(NULL, &ctx);
  if (res != TEEC_SUCCESS) {
    return -2;
  }

  res = TEEC_OpenSession(&ctx, &sess, &bkk_key_ta_uuid, TEEC_LOGIN_PUBLIC,
               NULL, NULL, &err_origin);
  if (res != TEEC_SUCCESS) {
    TEEC_FinalizeContext(&ctx);
    return -3;
  }

  memset(&op, 0, sizeof(op));
  op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE,
                   TEEC_NONE, TEEC_NONE);
  op.params[0].tmpref.buffer = buf;
  op.params[0].tmpref.size = *buf_len;

  res = TEEC_InvokeCommand(&sess, BKK_KEY_CMD_GET, &op, &err_origin);
  *buf_len = op.params[0].tmpref.size;

  TEEC_CloseSession(&sess);
  TEEC_FinalizeContext(&ctx);

  return (res == TEEC_SUCCESS) ? 0 : -4;
}
