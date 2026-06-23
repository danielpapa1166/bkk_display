// /data/projects/bkk_display/build-rpi/tmp/work/cortexa72-poky-linux/bkk-tee-ta/1.0-r0/

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <stddef.h>

#define BKK_KEY_OBJ_ID "bkk_api_key"
#define BKK_KEY_OBJ_ID_LEN (sizeof(BKK_KEY_OBJ_ID))

#define BKK_KEY_CMD_STORE               0U
#define BKK_KEY_CMD_GET                 1U
#define BKK_KEY_TEST_CMD                2U
#define BKK_KEY_ECHO_CMD                3U


static TEE_Result bkk_key_create_persistent_object(
    uint32_t param_types, TEE_Param params[4]) {

  const uint32_t exp_param_types = TEE_PARAM_TYPES(
    TEE_PARAM_TYPE_MEMREF_INPUT,
    TEE_PARAM_TYPE_MEMREF_INPUT,
    TEE_PARAM_TYPE_NONE,
    TEE_PARAM_TYPE_NONE);

  TEE_ObjectHandle object = TEE_HANDLE_NULL;
	TEE_Result res = TEE_ERROR_GENERIC;
	char *obj_id = NULL;
	size_t obj_id_sz = 0;
	char *data = NULL;
	size_t data_sz = 0;
	uint32_t obj_data_flag = 0;

  if (param_types != exp_param_types) {
    return TEE_ERROR_BAD_PARAMETERS;
  }

  // --------------------------------------------------------------------------
  // store the object ID from the parameters
  // --------------------------------------------------------------------------
  obj_id_sz = params[0].memref.size;
  obj_id = TEE_Malloc(obj_id_sz, 0);
  if (!obj_id) {
    return TEE_ERROR_OUT_OF_MEMORY;
  }

  TEE_MemMove(obj_id, params[0].memref.buffer, obj_id_sz);

  // --------------------------------------------------------------------------
  // store the data from the parameters
  // --------------------------------------------------------------------------
  data_sz = params[1].memref.size;
  data = TEE_Malloc(data_sz, 0);
  if (!data) {
    TEE_Free(obj_id);
    return TEE_ERROR_OUT_OF_MEMORY;
  }

  TEE_MemMove(data, params[1].memref.buffer, data_sz);

  // --------------------------------------------------------------------------
  // create the persistent object
  // --------------------------------------------------------------------------
  obj_data_flag = 
    TEE_DATA_FLAG_ACCESS_READ |
    TEE_DATA_FLAG_ACCESS_WRITE |
    TEE_DATA_FLAG_ACCESS_WRITE_META |
    TEE_DATA_FLAG_OVERWRITE;

  res = TEE_CreatePersistentObject(
    TEE_STORAGE_PRIVATE,
    obj_id,
    obj_id_sz,
    obj_data_flag,
    TEE_HANDLE_NULL,
    NULL, 
    0, 
    &object);

  if (res != TEE_SUCCESS) {
    TEE_Free(obj_id);
    TEE_Free(data);
    return res;
  }

  // --------------------------------------------------------------------------
  // write the data to the persistent object
  // --------------------------------------------------------------------------
  
  res = TEE_WriteObjectData(object, data, data_sz);
  if (res != TEE_SUCCESS) {
    TEE_CloseAndDeletePersistentObject1(object);
    TEE_Free(obj_id);
    TEE_Free(data);
    return res;
  }

  TEE_CloseObject(object);
  TEE_Free(obj_id);
  TEE_Free(data);
  return TEE_SUCCESS;
} 


static TEE_Result bkk_key_read_persistent_object(uint32_t param_types, TEE_Param params[4]) {

  const uint32_t exp_param_types = TEE_PARAM_TYPES(
    TEE_PARAM_TYPE_MEMREF_INPUT,
    TEE_PARAM_TYPE_MEMREF_OUTPUT,
    TEE_PARAM_TYPE_NONE,
    TEE_PARAM_TYPE_NONE);

  
  char *obj_id = NULL;
	size_t obj_id_sz = 0;
	char *data = NULL;
	size_t data_sz = 0;
  size_t actual_len = 0;


  TEE_ObjectHandle obj = TEE_HANDLE_NULL;
  TEE_ObjectInfo info;
  TEE_Result res;

  if (param_types != exp_param_types) {
    return TEE_ERROR_BAD_PARAMETERS;
  }

  // --------------------------------------------------------------------------
  // store the object ID from the parameters
  // --------------------------------------------------------------------------
  obj_id_sz = params[0].memref.size;
  obj_id = TEE_Malloc(obj_id_sz, 0);
  if (!obj_id) {
    return TEE_ERROR_OUT_OF_MEMORY;
  }

  TEE_MemMove(obj_id, params[0].memref.buffer, obj_id_sz);


  // --------------------------------------------------------------------------
  // prepare data container for output
  // --------------------------------------------------------------------------

  data_sz = params[1].memref.size;
	data = TEE_Malloc(data_sz, 0);
	if (!data) {
		TEE_Free(obj_id);
		return TEE_ERROR_OUT_OF_MEMORY;
	}

  // --------------------------------------------------------------------------
  // open the persistent object for reading
  // --------------------------------------------------------------------------

  res = TEE_OpenPersistentObject(
    TEE_STORAGE_PRIVATE,
    obj_id,
    obj_id_sz,
    TEE_DATA_FLAG_ACCESS_READ |
    TEE_DATA_FLAG_SHARE_READ,
    &obj);

  if (res != TEE_SUCCESS) {
    TEE_Free(obj_id);
    TEE_Free(data);

    return res;
  }

  // --------------------------------------------------------------------------
  // get the object info to determine the size of the data
  // --------------------------------------------------------------------------

  res = TEE_GetObjectInfo1(obj, &info);
  if (res != TEE_SUCCESS) {
    TEE_CloseObject(obj);
    TEE_Free(obj_id);
    TEE_Free(data);
    return res;
  }

  if (info.dataSize > data_sz) {
    TEE_CloseObject(obj);
    params[1].memref.size = info.dataSize;

    TEE_Free(obj_id);
    TEE_Free(data);

    return TEE_ERROR_SHORT_BUFFER;
  }

  // --------------------------------------------------------------------------
  // read the data from the persistent object
  // --------------------------------------------------------------------------

  uint32_t read_len = 0; 
  res = TEE_ReadObjectData(obj, data, info.dataSize, &read_len);
  actual_len = (size_t)read_len;

  if (res != TEE_SUCCESS) {
    TEE_CloseObject(obj);
    TEE_Free(obj_id);
    TEE_Free(data);
    return res;
  }


  // --------------------------------------------------------------------------
  // copy the data to the output parameter
  // --------------------------------------------------------------------------

  TEE_MemMove(params[1].memref.buffer, data, actual_len);
  params[1].memref.size = actual_len;

  TEE_Free(obj_id);
  TEE_Free(data);

  TEE_CloseObject(obj);
  return res;
}

TEE_Result TA_CreateEntryPoint(void) {
  return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void) {
}

TEE_Result TA_OpenSessionEntryPoint(
    uint32_t param_types, TEE_Param params[4], void **session_context) {
  
  (void)param_types;
  (void)params;
  (void)session_context;
  return TEE_SUCCESS;

}


void TA_CloseSessionEntryPoint(void *session_context) {
  (void)session_context;
}


TEE_Result TA_InvokeCommandEntryPoint(
    void *session_context, uint32_t cmd_id,
    uint32_t param_types, TEE_Param params[4]) {
                                      
  (void)session_context;

  switch (cmd_id) {
  case BKK_KEY_CMD_STORE: {
    return bkk_key_create_persistent_object(param_types, params);
  }
  case BKK_KEY_CMD_GET: {
    return bkk_key_read_persistent_object(param_types, params);
  }
  case BKK_KEY_TEST_CMD: {
    return TEE_SUCCESS;
  }
  case BKK_KEY_ECHO_CMD: {
    uint32_t expected = TEE_PARAM_TYPES(
      TEE_PARAM_TYPE_MEMREF_INPUT,
      TEE_PARAM_TYPE_MEMREF_OUTPUT,
      TEE_PARAM_TYPE_NONE,
      TEE_PARAM_TYPE_NONE);
    if (param_types != expected) {
      return TEE_ERROR_BAD_PARAMETERS;
    }
    if (params[0].memref.size > params[1].memref.size) {
      return TEE_ERROR_SHORT_BUFFER;
    }
    TEE_MemMove(params[1].memref.buffer, params[0].memref.buffer, params[0].memref.size);
    params[1].memref.size = params[0].memref.size;
    return TEE_SUCCESS;
  }
  default:
    return TEE_ERROR_NOT_SUPPORTED;
  }
}