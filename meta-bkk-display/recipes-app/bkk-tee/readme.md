# BKK TEE

`bkk-tee` provides the BKK secure-world integration. It packages an OP-TEE
trusted application together with a normal-world shared client library so
other applications can perform operations that must remain inside the Trusted
Execution Environment.

## Architecture

The build contains three parts:

- `src/trusted_app` builds the OP-TEE trusted application (`.ta`) with the
  OP-TEE TA development kit.
- `src/client` builds `libbkk-tee-client.so`, the normal-world API used by
  applications such as setup and main-content.
- `test` builds a client test target for development validation.

The two sides share protocol and public definitions in `include/bkk_tee`.

## Integration

`bkk-tee.bb` cross-builds the TA with `optee-os-tadevkit` and the client library
with CMake. The recipe installs the TA under `/lib/optee_armtz/`, where OP-TEE
loads it, and installs the client library plus development headers. Consumers
list `bkk-tee` in their build and runtime dependencies; rbuflogd supplies its
logging interface.