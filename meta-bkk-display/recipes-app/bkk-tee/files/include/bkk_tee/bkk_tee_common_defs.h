#ifndef BKK_TEE_COMMON_DEFS_H
#define BKK_TEE_COMMON_DEFS_H

// ----------------------------------------------------------------------------
// command ID definition: 
// ----------------------------------------------------------------------------

#define BKK_TEE_CMD_STORE               0U
#define BKK_TEE_CMD_GET                 1U

typedef enum {
  tee_object_type_api_key = 0,
  tee_object_type_wifi_pw = 1,
} tee_object_type_t;

#define OBJ_ID_KEY                      tee_object_type_api_key
#define OBJ_ID_WIFI_PW                  tee_object_type_wifi_pw

#define BKK_TEE_OBJ_CMD(obj_id, cmd_id)     \
  ((uint32_t)(((uint32_t)(obj_id) << 1) | (uint32_t)(cmd_id)))


#define BKK_KEY_CMD_STORE               BKK_TEE_OBJ_CMD(OBJ_ID_KEY, BKK_TEE_CMD_STORE)
#define BKK_KEY_CMD_GET                 BKK_TEE_OBJ_CMD(OBJ_ID_KEY, BKK_TEE_CMD_GET)
#define BKK_WIFI_PW_CMD_STORE           BKK_TEE_OBJ_CMD(OBJ_ID_WIFI_PW, BKK_TEE_CMD_STORE)
#define BKK_WIFI_PW_CMD_GET             BKK_TEE_OBJ_CMD(OBJ_ID_WIFI_PW, BKK_TEE_CMD_GET)

#define BKK_TEE_TEST_CMD                0xA0U
#define BKK_TEE_ECHO_CMD                0xA1U

#define BKK_TEE_MAX_OBJ_ID_LEN          125

#endif