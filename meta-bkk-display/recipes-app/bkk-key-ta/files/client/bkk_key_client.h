#ifndef BKK_KEY_CLIENT_H
#define BKK_KEY_CLIENT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int bkk_key_test(void);
int bkk_key_store(const void *key, size_t key_len);
int bkk_key_get(void *buf, size_t *buf_len);

#ifdef __cplusplus
}
#endif

#endif
