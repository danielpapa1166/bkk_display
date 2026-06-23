#ifndef BKK_TEE_CLIENT_H
#define BKK_TEE_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stddef.h>
#include <stdint.h>

int bkk_key_test(void);
int bkk_key_echo(const void *in, size_t in_len, void *out, size_t *out_len); 
int bkk_key_store(const void *key, size_t key_len);
int bkk_key_get(void *buf, size_t *buf_len);

#ifdef __cplusplus
}
#endif

#endif
