# bkk-key-ta Trusted Application Build and Install Flow

This document explains the build and install process implemented by the files in this directory:

- `CMakeLists.txt`
- `Makefile`
- `sub.mk`
- `user_ta_header_defines.h`

## 1. High-Level Architecture

The build is intentionally split into two layers:

1. CMake orchestration layer (`CMakeLists.txt`)
2. OP-TEE TA build system layer (`Makefile` + `sub.mk` + OP-TEE `ta_dev_kit.mk`)

In practice, CMake does not compile C code directly for this TA. Instead, CMake creates a custom command that invokes GNU Make in this directory, while injecting the OP-TEE-specific environment variables expected by `ta_dev_kit.mk`.

## 2. What CMake Requires

`CMakeLists.txt` defines the following cache variables:

- `BKK_KEY_TA_UUID` (default: `8f6f7b8a-21a4-4de8-9b8d-7c0b96648c19`)
  - Used as the TA output filename stem (`<uuid>.ta`)
- `OPTEE_TA_DEV_KIT_DIR` (required)
  - Path to OP-TEE exported user TA dev kit (the folder that contains `mk/ta_dev_kit.mk`)
- `TA_CROSS_COMPILE` (optional, but usually required in cross builds)
  - Toolchain prefix, for example `aarch64-linux-gnu-`
- `TA_SYSROOT` (required)
  - Sysroot path used to resolve runtime/compiler support libraries (`libgcc`, etc.)

Hard checks:

- If `OPTEE_TA_DEV_KIT_DIR` is empty, configuration fails with a fatal error.
- If `TA_SYSROOT` is empty, configuration fails with a fatal error.

## 3. Build Directories and Output

From CMake:

- Build output staging directory:
  - `${CMAKE_CURRENT_BINARY_DIR}/ta-build`
- TA output artifact:
  - `${CMAKE_CURRENT_BINARY_DIR}/ta-build/${BKK_KEY_TA_UUID}.ta`

This means the final TA file is generated in a CMake binary tree subdirectory, not in the source directory.

## 4. How CMake Invokes Make

The `add_custom_command(...)` in `CMakeLists.txt` performs:

1. Create TA build directory (`ta-build`)
2. Run:

```sh
make -C <trusted_app_source_dir>
```

with environment variables injected using `cmake -E env`:

- `TA_DEV_KIT_DIR=${OPTEE_TA_DEV_KIT_DIR}`
- `BINARY=${BKK_KEY_TA_UUID}`
- `O=${TA_BUILD_DIR}`
- `CROSS_COMPILE=${TA_CROSS_COMPILE}`
- `LIBGCC_LOCATE_CFLAGS=--sysroot=${TA_SYSROOT}`
- `CFLAGS32=--sysroot=${TA_SYSROOT}`
- `CFLAGS64=--sysroot=${TA_SYSROOT}`

### Why these matter

- `TA_DEV_KIT_DIR` allows this local `Makefile` to include OP-TEE's `mk/ta_dev_kit.mk`.
- `BINARY` controls output TA name; this must match deployment/runtime expectations.
- `O` keeps generated objects and intermediate files in the out-of-tree build folder.
- `CROSS_COMPILE` selects the cross toolchain binaries.
- `LIBGCC_LOCATE_CFLAGS`, `CFLAGS32`, and `CFLAGS64` ensure compiler and linker sysroot visibility.

## 5. Makefile Role

`Makefile` is very small by design:

```make
BINARY ?= 8f6f7b8a-21a4-4de8-9b8d-7c0b96648c19

ifneq ($(TA_DEV_KIT_DIR),)
include $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk
else
$(error TA_DEV_KIT_DIR is not set)
endif
```

Behavior:

- Uses a default `BINARY` UUID if none is provided.
- Requires `TA_DEV_KIT_DIR` to be set.
- Delegates almost all TA build logic to OP-TEE's `ta_dev_kit.mk`.

Because CMake always sets `BINARY` and `TA_DEV_KIT_DIR` in the custom command environment, the CMake-driven path overrides defaults and provides deterministic behavior.

## 6. Source Registration via `sub.mk`

`sub.mk` contributes source files to the OP-TEE TA build:

```make
srcs-y += bkk_key_ta.c
```

This is the source list consumed by `ta_dev_kit.mk`.

## 7. Header Defines and UUID Consistency

`user_ta_header_defines.h` defines TA metadata, including the compile-time UUID used by the TA itself.

Important consistency rule:

- CMake output filename UUID (`BKK_KEY_TA_UUID`) should match
- `TA_UUID` in `user_ta_header_defines.h`

If these differ, the generated file name and embedded TA identity can become inconsistent, which may lead to TA loading/lookup failures depending on runtime expectations.

## 8. CMake Target Graph

`CMakeLists.txt` defines:

- Custom command output: `${TA_OUTPUT}`
- Custom target: `bkk-key-ta` (built by default via `ALL`)
- Install rule:

```cmake
install(FILES ${TA_OUTPUT} DESTINATION ${CMAKE_INSTALL_LIBDIR}/optee_armtz)
```

Consequences:

- Running a normal CMake build (`cmake --build ...`) builds this TA automatically.
- Running install (`cmake --install ...`) copies the generated `.ta` file into:
  - `<install-prefix>/${CMAKE_INSTALL_LIBDIR}/optee_armtz`

On many Linux targets, this often resolves to something like `lib/optee_armtz` under the selected install prefix.

## 9. End-to-End Command Sequence

Example (adapt paths/toolchain as needed):

```sh
cmake -S /data/projects/bkk_display/meta-bkk-display/recipes-app/bkk-key-ta/files/trusted_app \
      -B /tmp/bkk-key-ta-build \
      -DOPTEE_TA_DEV_KIT_DIR=/path/to/optee/export-ta_arm64 \
      -DTA_SYSROOT=/path/to/target-sysroot \
      -DTA_CROSS_COMPILE=aarch64-linux-gnu- \
      -DBKK_KEY_TA_UUID=8f6f7b8a-21a4-4de8-9b8d-7c0b96648c19

cmake --build /tmp/bkk-key-ta-build --target bkk-key-ta

cmake --install /tmp/bkk-key-ta-build --prefix /tmp/bkk-key-ta-install
```

Resulting install location:

```text
/tmp/bkk-key-ta-install/lib/optee_armtz/8f6f7b8a-21a4-4de8-9b8d-7c0b96648c19.ta
```

(`lib` may vary if `CMAKE_INSTALL_LIBDIR` is overridden or platform-adjusted.)

## 10. Incremental Rebuild Triggers

CMake custom command dependencies include:

- `bkk_key_ta.c`
- `Makefile`
- `sub.mk`
- `user_ta_header_defines.h`

Changes to any of these trigger rebuild of the TA output.

## 11. Common Failure Modes

1. `OPTEE_TA_DEV_KIT_DIR` missing or wrong
   - CMake configure fails, or make include path fails.
2. `TA_SYSROOT` missing
   - CMake configure fails immediately.
3. Wrong `TA_CROSS_COMPILE`
   - Compiler tools not found or wrong architecture artifacts.
4. UUID mismatch between CMake and header
   - TA may be installed under one UUID filename while internally declaring another.

## 12. Quick Verification Checklist

After build/install, verify:

1. `${build_dir}/ta-build/<uuid>.ta` exists.
2. Installed file exists in `<prefix>/<libdir>/optee_armtz/<uuid>.ta`.
3. `<uuid>` matches both:
   - `BKK_KEY_TA_UUID` used at CMake configure time
   - `TA_UUID` in `user_ta_header_defines.h`
