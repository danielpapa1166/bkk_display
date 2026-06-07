# OP-TEE API Key Protection

Plan for protecting the BKK API key using OP-TEE TrustZone on the RPi 4.

## Why

The API key is entered by the user during the config-server setup phase and is
currently written to `/etc/bkk-api/api-key.txt` (plaintext). The goal is to
store it in OP-TEE Secure Storage instead — encrypted with a Hardware Unique
Key (HUK) that never leaves the secure world — so the key cannot be extracted
by reading the filesystem or the SD card.

## Architecture

```
Normal World (Linux)                     Secure World (OP-TEE)
─────────────────────────────────────    ──────────────────────────────
config-server  ──store──►               bkk-key TA
bkk_uds_server ──get────►  libteec  →   └─ Secure Storage
                           tee-supplicant   (AES-GCM, HUK-derived key)
```

## Components to build

| Component | Location | Description |
|-----------|----------|-------------|
| `bkk-key TA` | `submodules/bkk-key-ta/ta/` | Trusted Application: two commands — `STORE` and `GET` |
| `libbkk-key-client` | `submodules/bkk-key-ta/host/` | Normal-world client library (wraps `libteec`) |
| `bkk-key-ta.bb` | `meta-bkk-display/recipes-api/bkk-key-ta/` | Yocto recipe — builds and installs the TA to `/lib/optee_armtz/` |
| `bkk-key-client.bb` | `meta-bkk-display/recipes-api/bkk-key-client/` | Yocto recipe — builds `libbkk-key-client.so` |

## Changes to existing code

- **config-server** — Phase 2 handler: replace `write key to file` with `bkk_key_store(key, len)`
- **bkk_uds_server** — startup: replace reading key from file/env with `bkk_key_get(buf, len)`, wipe buffer after use

## Yocto build changes

- Add `meta-arm/meta-arm` and `meta-arm/meta-arm-toolchain` to `bblayers.conf`
- Set `DISTRO_FEATURES:append = " optee"` in `local.conf`
- Add `optee-client` (provides `tee-supplicant` + `libteec`) to the image

## Boot chain

OP-TEE requires replacing the standard RPi 4 boot chain:

```
standard:    firmware → arm-stub → kernel
with OP-TEE: firmware → TF-A (BL31) → OP-TEE OS (BL32) → U-Boot (BL33) → kernel
```

Getting the boot chain correct is the highest-effort part of the integration.
Verify it works first using `tee-supplicant -d` and the `xtest` suite from
`optee-test` before building the TA.

## Key lifecycle after integration

1. User enters key in browser (config-server Phase 2)
2. `config-server` calls `bkk_key_store()` → key written to Secure Storage
3. Plaintext key never touches the Linux filesystem
4. At startup, `bkk_uds_server` calls `bkk_key_get()` → key returned in memory only
5. Key is appended to the curl request URL, then wiped from memory

## Minimal implementation example

The smallest useful TA for this use case has only two commands:

- `CMD_STORE_KEY`: write key bytes to OP-TEE secure storage
- `CMD_GET_KEY`: read key bytes back into a caller-provided output buffer

### Trusted Application (Secure World)

Suggested file: `submodules/bkk-key-ta/ta/bkk_key_ta.c`

```c
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#define BKK_KEY_OBJ_ID "bkk_api_key"
#define BKK_KEY_OBJ_ID_LEN 11

#define CMD_STORE_KEY 0
#define CMD_GET_KEY   1

static TEE_Result store_key(const void *data, uint32_t len)
{
    TEE_ObjectHandle obj = TEE_HANDLE_NULL;
    uint32_t flags = TEE_DATA_FLAG_ACCESS_WRITE |
                     TEE_DATA_FLAG_ACCESS_READ |
                     TEE_DATA_FLAG_ACCESS_WRITE_META |
                     TEE_DATA_FLAG_OVERWRITE;

    TEE_Result res = TEE_CreatePersistentObject(
        TEE_STORAGE_PRIVATE,
        BKK_KEY_OBJ_ID, BKK_KEY_OBJ_ID_LEN,
        flags,
        TEE_HANDLE_NULL,
        NULL, 0,
        &obj);
    if (res != TEE_SUCCESS)
        return res;

    res = TEE_WriteObjectData(obj, data, len);
    TEE_CloseObject(obj);
    return res;
}

static TEE_Result get_key(void *out, uint32_t out_len, uint32_t *actual_len)
{
    TEE_ObjectHandle obj = TEE_HANDLE_NULL;
    TEE_ObjectInfo info;
    TEE_Result res = TEE_OpenPersistentObject(
        TEE_STORAGE_PRIVATE,
        BKK_KEY_OBJ_ID, BKK_KEY_OBJ_ID_LEN,
        TEE_DATA_FLAG_ACCESS_READ,
        &obj);
    if (res != TEE_SUCCESS)
        return res;

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

TEE_Result TA_InvokeCommandEntryPoint(void *sess_ctx, uint32_t cmd_id,
                                      uint32_t param_types,
                                      TEE_Param params[4])
{
    (void)sess_ctx;

    switch (cmd_id) {
    case CMD_STORE_KEY: {
        uint32_t exp = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                                       TEE_PARAM_TYPE_NONE,
                                       TEE_PARAM_TYPE_NONE,
                                       TEE_PARAM_TYPE_NONE);
        if (param_types != exp)
            return TEE_ERROR_BAD_PARAMETERS;
        return store_key(params[0].memref.buffer, params[0].memref.size);
    }
    case CMD_GET_KEY: {
        uint32_t exp = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
                                       TEE_PARAM_TYPE_NONE,
                                       TEE_PARAM_TYPE_NONE,
                                       TEE_PARAM_TYPE_NONE);
        if (param_types != exp)
            return TEE_ERROR_BAD_PARAMETERS;
        return get_key(params[0].memref.buffer,
                       params[0].memref.size,
                       &params[0].memref.size);
    }
    default:
        return TEE_ERROR_NOT_SUPPORTED;
    }
}
```

### Client library (Normal World)

Suggested file: `submodules/bkk-key-ta/host/bkk_key_client.c`

```c
#include <string.h>
#include <tee_client_api.h>

#define CMD_STORE_KEY 0
#define CMD_GET_KEY   1

/* Replace with your real TA UUID from user_ta_header_defines.h */
static const TEEC_UUID BKK_KEY_TA_UUID =
    { 0x12345678, 0x1234, 0x1234,
      { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };

int bkk_key_store(const void *key, size_t key_len)
{
    TEEC_Context ctx;
    TEEC_Session sess;
    TEEC_Operation op;
    uint32_t err_origin;
    TEEC_Result res;

    res = TEEC_InitializeContext(NULL, &ctx);
    if (res != TEEC_SUCCESS)
        return -1;

    res = TEEC_OpenSession(&ctx, &sess, &BKK_KEY_TA_UUID,
                           TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
    if (res != TEEC_SUCCESS) {
        TEEC_FinalizeContext(&ctx);
        return -2;
    }

    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_NONE, TEEC_NONE, TEEC_NONE);
    op.params[0].tmpref.buffer = (void *)key;
    op.params[0].tmpref.size = key_len;

    res = TEEC_InvokeCommand(&sess, CMD_STORE_KEY, &op, &err_origin);
    TEEC_CloseSession(&sess);
    TEEC_FinalizeContext(&ctx);
    return (res == TEEC_SUCCESS) ? 0 : -3;
}

int bkk_key_get(void *buf, size_t *buf_len)
{
    TEEC_Context ctx;
    TEEC_Session sess;
    TEEC_Operation op;
    uint32_t err_origin;
    TEEC_Result res;

    res = TEEC_InitializeContext(NULL, &ctx);
    if (res != TEEC_SUCCESS)
        return -1;

    res = TEEC_OpenSession(&ctx, &sess, &BKK_KEY_TA_UUID,
                           TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
    if (res != TEEC_SUCCESS) {
        TEEC_FinalizeContext(&ctx);
        return -2;
    }

    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_NONE, TEEC_NONE, TEEC_NONE);
    op.params[0].tmpref.buffer = buf;
    op.params[0].tmpref.size = *buf_len;

    res = TEEC_InvokeCommand(&sess, CMD_GET_KEY, &op, &err_origin);
    *buf_len = op.params[0].tmpref.size;

    TEEC_CloseSession(&sess);
    TEEC_FinalizeContext(&ctx);
    return (res == TEEC_SUCCESS) ? 0 : -3;
}
```

## Integration steps for bkk_display

These steps assume your OP-TEE platform is already up (`/dev/tee0` exists and
`xtest` passes).

1. Add TA source tree

- Create TA project under `submodules/bkk-key-ta/` with:
  - `ta/` (secure app source, UUID header, TA dev-kit makefile)
  - `host/` (normal-world client lib using `libteec`)

2. Create Yocto recipe for TA package

- Add recipe directory:
  - `meta-bkk-display/recipes-api/bkk-key-ta/`
- Recipe should install built `.ta` into:
  - `/lib/optee_armtz/<ta_uuid>.ta`

3. Create Yocto recipe for client library

- Add recipe directory:
  - `meta-bkk-display/recipes-api/bkk-key-client/`
- Build/install `libbkk-key-client.so` and headers for app linking.

4. Add packages to image

- Update image append (currently in this repo at
  `meta-bkk-display/recipes-core/images/core-image-full-cmdline.bbappend`) to include:
  - `bkk-key-ta`
  - `bkk-key-client`
  - `optee-client`

5. Wire config-server

- Replace plaintext file write flow with:
  - `bkk_key_store(api_key, key_len)`
- On success, clear input buffers with explicit memory wipe.

6. Wire bkk_uds_server

- Replace key file load with:
  - `bkk_key_get(buf, &len)`
- Use key only in-memory, then wipe immediately after request signing/curl use.

7. Build and deploy

- Build image, flash, boot.
- Verify runtime:
  - `systemctl status tee-supplicant`
  - `ls -l /dev/tee*`
  - `xtest`

8. Validate TA end-to-end

- Write a tiny host test tool that:
  - stores a known key via TA
  - reads it back
  - compares bytes
- Confirm no key file appears under `/etc/bkk-api/`.

## Security notes

- Keep key length bounded (for example max 256 bytes).
- Use strict parameter checks in TA (`param_types`, null checks, size checks).
- Prefer returning generic errors to normal world.
- Wipe sensitive buffers in both TA and client code.
- Add rate limiting or lockout in normal-world API path to reduce abuse.
