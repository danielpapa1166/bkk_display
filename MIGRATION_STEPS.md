# Migration Steps — Option A

Target layout:

```
bkk_display/
  submodules/
    bkk_api/        (moved submodule)
    bkk_qt_app/     (new repo, extracted from recipe)
    bkk_web_setup/  (new repo, extracted from recipe)
    chttp/          (moved submodule)
    cJSON/          (moved submodule)
    rbuflogd/       (moved submodule)
  meta-bkk/         (new, consolidates meta-BKK-api + meta-bkk-setup + meta-display-config)
  meta-platform-config/   (submodule, unchanged)
  meta-Qt/          (unchanged)
  build-rpi/        (unchanged)
```

---

## Step 1 — Move existing submodules under `submodules/`

This is a path-only rename inside `.gitmodules`. The remote URLs do not change.

```bash
# Move the working tree
git mv bkk_api    submodules/bkk_api
git mv chttp      submodules/chttp
git mv cJSON      submodules/cJSON
git mv rbuflogd   submodules/rbuflogd

# Update .gitmodules paths to match
# Change:  path = bkk_api     → path = submodules/bkk_api
# Change:  path = chttp       → path = submodules/chttp
# Change:  path = cJSON       → path = submodules/cJSON
# Change:  path = rbuflogd    → path = submodules/rbuflogd

git add .gitmodules
git commit -m "refactor: regroup submodules under submodules/"
```

---

## Step 2 — Extract Qt app source into a standalone git repository

The Qt application source currently lives inside a Yocto recipe and has no independent version history.

```bash
# 1. Create the new repo locally and push to remote
mkdir -p /tmp/bkk_qt_app
cp -r meta-Qt/recipes-Qt/Bkk_Qt_App/files/src/. /tmp/bkk_qt_app/
cd /tmp/bkk_qt_app
git init && git add . && git commit -m "initial: extract from bkk_display recipe"
# push to e.g. github.com/danielpapa1166/bkk_qt_app

# 2. Add as a submodule in bkk_display
cd /data/projects/bkk_display
git submodule add git@github.com:danielpapa1166/bkk_qt_app.git submodules/bkk_qt_app
git submodule add git@github.com:danielpapa1166/bkk_web_setup.git submodules/bkk_web_setup

# 3. Remove the now-redundant source copies from the recipes
#    (the recipe's SRC_URI will be updated in Step 5 to point to EXTERNALSRC)
git rm -r meta-Qt/recipes-Qt/Bkk_Qt_App/files/src
git rm -r meta-Qt/recipes-Qt/Bkk_Web_Setup_Helper/files/src

git commit -m "refactor: extract Qt app source into standalone repos"
```

---

## Step 3 — Fix `cJSON`/`rbuflogd` double-checkout inside `bkk_api`

`bkk_api` registers `cJSON` and `rbuflogd` as its own nested submodules. With Step 1 done,
the top-level copies under `submodules/` are authoritative. Remove the nesting.

This change is made in the **`bkk_api` repository**:

```bash
cd submodules/bkk_api

# Remove the nested submodule entries
git submodule deinit submodules/cJSON
git submodule deinit submodules/rbuflogd
git rm submodules/cJSON
git rm submodules/rbuflogd
# Edit .gitmodules to remove those two entries
git add .gitmodules
git commit -m "refactor: remove nested cJSON/rbuflogd submodules (provided by parent)"
```

Then update `bkk_api`'s CMake to accept the paths as variables from the parent build system:

```cmake
# In bkk_api/CMakeLists.txt — replace hardcoded submodule paths with:
set(CJSON_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/../cJSON"    CACHE PATH "cJSON source dir")
set(RBUFLOGD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../rbuflogd" CACHE PATH "rbuflogd source dir")
```

The top-level Yocto `bkk-api` recipe already fetches `cJSON` and `rbuflogd` independently
(see `SRC_URI` in `meta-BKK-api/recipes-api/bkk-api/`), so the Yocto side needs no change
beyond the SRCREV pins for `bkk_api` once the nested submodules are gone.

---

## Step 4 — Create `meta-bkk` and migrate recipes

Create the new consolidated layer, then move recipe directories into it.

```bash
mkdir -p meta-bkk/conf
# Copy layer.conf from any existing layer and update BBFILE_COLLECTIONS name
cp meta-BKK-api/conf/layer.conf meta-bkk/conf/layer.conf
# Edit: set BBFILE_COLLECTIONS = "bkk" and adjust BBFILE_PATTERN/priority as needed
```

Move recipe directories:

| Source | Destination in `meta-bkk/` |
|---|---|
| `meta-BKK-api/recipes-api/` | `recipes-api/` |
| `meta-BKK-api/recipes-core/` | `recipes-core/` (merge with below) |
| `meta-bkk-setup/recipes-libs/` | `recipes-libs/` |
| `meta-bkk-setup/recipes-setup/` | `recipes-setup/` |
| `meta-display-config/recipes-apps/` | `recipes-apps/` |
| `meta-display-config/recipes-libs/` | `recipes-libs/` (merge with above) |
| `meta-display-config/recipes-bsp/` | `recipes-bsp/` |
| `meta-display-config/recipes-core/` | `recipes-core/` (merge with above) |
| `meta-display-config/recipes-kernel/` | `recipes-kernel/` |

```bash
git mv meta-BKK-api/recipes-api       meta-bkk/recipes-api
git mv meta-bkk-setup/recipes-libs    meta-bkk/recipes-libs
git mv meta-bkk-setup/recipes-setup   meta-bkk/recipes-setup
git mv meta-display-config/recipes-apps    meta-bkk/recipes-apps
git mv meta-display-config/recipes-bsp     meta-bkk/recipes-bsp
git mv meta-display-config/recipes-kernel  meta-bkk/recipes-kernel
# Merge the two recipes-core and recipes-libs directories manually, then:
git rm -r meta-BKK-api meta-bkk-setup meta-display-config

git commit -m "refactor: consolidate meta-BKK-api, meta-bkk-setup, meta-display-config into meta-bkk"
```

---

## Step 5 — Update Qt app recipes to use `EXTERNALSRC`

Once the Qt app source is a checked-out submodule at `submodules/bkk_qt_app/`, the recipe
should point directly at it rather than fetching via `file://`. This avoids copying source
into `WORKDIR` on every build.

In `meta-Qt/recipes-Qt/Bkk_Qt_App/bkk-qt-app_1.0.bb` (and the helper recipe):

```bitbake
# Remove the SRC_URI file:// block and S = "${WORKDIR}/src" lines.
# Add:
inherit externalsrc
EXTERNALSRC = "${TOPDIR}/../submodules/bkk_qt_app"
EXTERNALSRC_BUILD = "${WORKDIR}/build"
```

> **Note:** `inherit externalsrc` disables Yocto's source fetching and unpacking entirely.
> Changes in the submodule directory are picked up on the next `bitbake` run. This is also
> convenient for active development: edit source in `submodules/bkk_qt_app/`, run bitbake.

---

## Step 6 — Update `bblayers.conf`

Replace the three old layer paths with the single new one:

```bitbake
# Remove:
#   /data/projects/bkk_display/meta-BKK-api
#   /data/projects/bkk_display/meta-bkk-setup
#   /data/projects/bkk_display/meta-display-config
# Add:
#   /data/projects/bkk_display/meta-bkk

BBLAYERS ?= " \
  /data/projects/yocto/poky/meta \
  /data/projects/yocto/poky/meta-poky \
  /data/projects/yocto/poky/meta-yocto-bsp \
  /data/projects/yocto/meta-raspberrypi \
  /data/projects/yocto/meta-openembedded/meta-oe \
  /data/projects/bkk_display/meta-platform-config \
  /data/projects/bkk_display/meta-bkk \
  /data/projects/bkk_display/meta-Qt \
  /data/projects/yocto/meta-qt5 \
  /data/projects/bkk_display/build-rpi/workspace \
  "
```

---

## Step 7 — Update the `bkk-api` recipe `SRC_URI` path

After Step 1 the `bkk_api` submodule lives at `submodules/bkk_api`. The recipe currently
references `${TOPDIR}/../bkk_api`:

In `meta-bkk/recipes-api/bkk-api/bkk-api_*.bb`:

```bitbake
# Change:
SRC_URI = "git://${TOPDIR}/../bkk_api;protocol=file;...
# To:
SRC_URI = "git://${TOPDIR}/../submodules/bkk_api;protocol=file;...
```

---

## Verification checklist

- [ ] `git submodule status` shows all five submodules at their new paths with no `+` or `-` prefix
- [ ] `bitbake bkk-api` builds cleanly from `submodules/bkk_api`
- [ ] `bitbake bkk-qt-app` builds cleanly using `EXTERNALSRC`
- [ ] `bitbake core-image-full-cmdline` (or your image target) completes without errors
- [ ] No leftover directories at root: `bkk_api/`, `chttp/`, `cJSON/`, `rbuflogd/` are gone
- [ ] `meta-BKK-api/`, `meta-bkk-setup/`, `meta-display-config/` are removed
