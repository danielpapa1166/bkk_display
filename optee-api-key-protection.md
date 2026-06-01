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
