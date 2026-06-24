#ifndef BKK_TEE_CLIENT_H
#define BKK_TEE_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stddef.h>
#include <stdint.h>

#include "bkk_tee_common_defs.h"


int bkk_tee_test(void);
int bkk_tee_echo(const void *in, size_t in_len, void *out, size_t *out_len); 
int bkk_tee_store(tee_object_type_t obj_type, const void *key, size_t key_len);
int bkk_tee_get(tee_object_type_t obj_type, void *buf, size_t *buf_len);

#ifdef __cplusplus
}
#endif

#endif
