#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <stddef.h>

#define BKK_KEY_OBJ_ID "bkk_api_key"
#define BKK_KEY_OBJ_ID_LEN (sizeof(BKK_KEY_OBJ_ID) - 1U)

#define BKK_KEY_CMD_STORE 0U
#define BKK_KEY_CMD_GET   1U
#define BKK_KEY_TEST_CMD  2U

static TEE_Result store_key(const void * data, size_t data_len) {
  TEE_ObjectHandle obj = TEE_HANDLE_NULL;
  uint32_t flags = TEE_DATA_FLAG_ACCESS_WRITE |
                   TEE_DATA_FLAG_ACCESS_READ |
                   TEE_DATA_FLAG_ACCESS_WRITE_META |
                   TEE_DATA_FLAG_OVERWRITE;
  TEE_Result res;

  res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
                                   BKK_KEY_OBJ_ID,
                                   BKK_KEY_OBJ_ID_LEN,
                                   flags,
                                   TEE_HANDLE_NULL,
                                   NULL,
                                   0,
                                   &obj);
  if (res != TEE_SUCCESS) {
    return res;
  }

  res = TEE_WriteObjectData(obj, data, data_len);
  TEE_CloseObject(obj);
  return res;
}

static TEE_Result get_key(void *out, size_t out_len, uint32_t *actual_len) {
  TEE_ObjectHandle obj = TEE_HANDLE_NULL;
  TEE_ObjectInfo info;
  TEE_Result res;

  res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
                                 BKK_KEY_OBJ_ID,
                                 BKK_KEY_OBJ_ID_LEN,
                                 TEE_DATA_FLAG_ACCESS_READ,
                                 &obj);
  if (res != TEE_SUCCESS) {
    return res;
  }

  res = TEE_GetObjectInfo1(obj, &info);
  if (res != TEE_SUCCESS) {
    TEE_CloseObject(obj);
    return res;
  }

  if (info.dataSize > out_len) {
    TEE_CloseObject(obj);
    return TEE_ERROR_SHORT_BUFFER;
  }

  res = TEE_ReadObjectData(obj, out, out_len, actual_len);
  TEE_CloseObject(obj);
  return res;
}

TEE_Result TA_CreateEntryPoint(void) {
  return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void) {
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
                                    TEE_Param params[4],
                                    void **session_context) {
  (void)param_types;
  (void)params;
  (void)session_context;
  return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void *session_context) {
  (void)session_context;
}

TEE_Result TA_InvokeCommandEntryPoint(void *session_context,
                                      uint32_t cmd_id,
                                      uint32_t param_types,
                                      TEE_Param params[4]) {
  (void)session_context;

  switch (cmd_id) {
  case BKK_KEY_CMD_STORE: {
    uint32_t expected = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                                        TEE_PARAM_TYPE_NONE,
                                        TEE_PARAM_TYPE_NONE,
                                        TEE_PARAM_TYPE_NONE);
    if (param_types != expected) {
      return TEE_ERROR_BAD_PARAMETERS;
    }
    return store_key(params[0].memref.buffer, params[0].memref.size);
  }
  case BKK_KEY_CMD_GET: {
    uint32_t expected = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
                                        TEE_PARAM_TYPE_NONE,
                                        TEE_PARAM_TYPE_NONE,
                                        TEE_PARAM_TYPE_NONE);
    if (param_types != expected) {
      return TEE_ERROR_BAD_PARAMETERS;
    }
    return get_key(params[0].memref.buffer,
                   params[0].memref.size,
                   &params[0].memref.size);
  }
  case BKK_KEY_TEST_CMD: {
    return TEE_SUCCESS;
  }
  default:
    return TEE_ERROR_NOT_SUPPORTED;
  }
}