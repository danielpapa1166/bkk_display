#ifndef BKK_TEE_CLIENT_H
#define BKK_TEE_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stddef.h>
#include <stdint.h>

#include "bkk_tee/bkk_tee_common_defs.h"

typedef enum {
  bkk_tee_client_err_none = 0,
  bkk_tee_client_err_invalid_arg = -1,
  bkk_tee_client_err_session_failed = -2,
  bkk_tee_client_err_invoke_failed = -3,
  bkk_tee_client_err_unknown = -4
} bkk_tee_client_status_t;


bkk_tee_client_status_t bkk_tee_test(void);
bkk_tee_client_status_t bkk_tee_echo(const void *in, size_t in_len, void *out, size_t *out_len); 
bkk_tee_client_status_t bkk_tee_store(tee_object_type_t obj_type, const void *key, size_t key_len);
bkk_tee_client_status_t bkk_tee_get(tee_object_type_t obj_type, void *buf, size_t *buf_len);

#ifdef __cplusplus
}
#endif

#endif
