# Project Layout Proposals

## Background

The current root `.gitmodules` registers five submodules:

| Submodule | Remote | Notes |
|---|---|---|
| `bkk_api` | danielpapa1166/bkk_api | First-party C server; also has its own nested submodules (`cJSON`, `rbuflogd`) |
| `chttp` | danielpapa1166/chttp | First-party HTTP client library |
| `cJSON` | DaveGamble/cJSON | Third-party; also pinned inside `bkk_api` |
| `rbuflogd` | danielpapa1166/rbuflogd | First-party logging; also pinned inside `bkk_api` |
| `meta-platform-config` | danielpapa1166/meta-platform-config | BSP/platform layer; may be inlined |

Current pain points:

- `cJSON` and `rbuflogd` are checked out **twice** — once at the project root (for Yocto packaging) and once nested inside `bkk_api/submodules/` (for `bkk_api`'s own build). This is redundant.
- The Qt application source lives **inside** a Yocto recipe (`meta-Qt/recipes-Qt/Bkk_Qt_App/files/src/`) with no standalone git repo — it is the only first-party component that is not independently versioned.
- Five separate meta layers (`meta-BKK-api`, `meta-bkk-setup`, `meta-display-config`, `meta-Qt`, `meta-platform-config`) have overlapping and unclear responsibilities.
- All submodules, meta layers, and the build directory share the same flat root level with no grouping.

---

## Option A — Single `meta-bkk` layer + grouped submodule paths

Group all submodule checkouts under a `submodules/` directory (just a path change in `.gitmodules` — no repos change), extract the Qt app into its own repo, and collapse the five project-specific meta layers into one.

```
bkk_display/
  submodules/
    bkk_api/            ← submodule (danielpapa1166/bkk_api)
    bkk_qt_app/         ← NEW standalone repo (source extracted from recipe files/src/)
    chttp/              ← submodule (danielpapa1166/chttp)
    cJSON/              ← submodule (DaveGamble/cJSON)  — top-level only; drop from bkk_api
    rbuflogd/           ← submodule (danielpapa1166/rbuflogd) — top-level only; drop from bkk_api
  meta-bkk/             ← ONE project-specific layer (replaces meta-BKK-api, meta-bkk-setup,
    conf/                 meta-display-config)
    recipes-api/        ← bkk-uds-server, bkk-uds-client
    recipes-app/        ← bkk-qt-app, bkk-web-setup-helper
    recipes-libs/       ← cJSON, chttp, rbuflogd, ads7846-controller
    recipes-setup/      ← application-manager, config-server, key-env
    recipes-core/       ← image definition, systemd units
  meta-platform-config/ ← keep or inline (see note below)
  meta-Qt/              ← keep as-is: Qt version/config only
  build-rpi/
```

**Note on `meta-platform-config`:** If inlined, its recipes merge into `meta-bkk` (`recipes-kernel/`, `recipes-connectivity/`) and the submodule is removed. If kept as a submodule, it stays separate and can be shared with other projects.

**Note on `cJSON`/`rbuflogd` duplication:** Drop them from `bkk_api`'s own `submodules/` and have `bkk_api` reference the top-level copies via a relative path or CMake `find_package`. This eliminates the double checkout.

### Recipe responsibilities

| Layer | Contains |
|---|---|
| `meta-bkk` | Everything specific to this application and its dependencies |
| `meta-platform-config` | Kernel patches, connectivity, hardware-level BSP tweaks |
| `meta-Qt` | Qt version pinning/config, no application logic |

### Pros
- Single layer to touch for any application change
- Clean root: submodules grouped, build dir isolated
- `cJSON`/`rbuflogd` duplication eliminated
- Easy to navigate

### Cons
- `meta-bkk` becomes large; harder to share individual components later
- Board assumptions can silently accumulate in one layer

---

## Option B — Two-layer split: `meta-bkk-platform` + `meta-bkk-app`

Same submodule grouping as Option A, but split recipes along a **board vs. application** boundary. `meta-platform-config` is absorbed into `meta-bkk-platform` (ending its submodule lifetime).

```
bkk_display/
  submodules/
    bkk_api/
    bkk_qt_app/         ← NEW standalone repo
    chttp/
    cJSON/
    rbuflogd/
  meta-bkk-platform/    ← board-specific: absorbs meta-platform-config + Qt config
    conf/               ← replaces meta-platform-config submodule entirely
    recipes-bsp/
    recipes-connectivity/
    recipes-kernel/
    recipes-Qt/         ← Qt version/config (absorbed from meta-Qt)
  meta-bkk-app/         ← application-specific: all bkk recipes, libs, image
    conf/
    recipes-api/
    recipes-app/
    recipes-libs/
    recipes-setup/
    recipes-core/       ← image definition
  build-rpi/
```

### Layer responsibilities

| Layer | Contains | Portable to… |
|---|---|---|
| `meta-bkk-platform` | Kernel config, Qt version, BSP, connectivity | A different app on the same RPi4 |
| `meta-bkk-app` | BKK server, Qt app, libs, setup, image | A different board (swap platform layer) |

### Pros
- Clean portability boundary: new board = swap `meta-bkk-platform` only
- `meta-platform-config` submodule is replaced by an inline layer, removing one submodule dependency
- Each layer's responsibility is unambiguous

### Cons
- One extra layer to maintain and wire into `bblayers.conf`
- Slight overhead if the project will always target only one board

---

## Recommendation

For a **dedicated single-board appliance** with no near-term porting plans: go with **Option A**. It is simpler to maintain and the least overhead. Keep `meta-platform-config` as a submodule only if you intend to share it with other projects.

If there is any chance of **porting to another board**: go with **Option B** and inline `meta-platform-config` into `meta-bkk-platform`.

**The single highest-value first step is the same in both options:** extract the Qt application source from `meta-Qt/recipes-Qt/Bkk_Qt_App/files/src/` into its own standalone git repository. It is the only first-party component without independent versioning, and fixing this unlocks a clean submodule grouping for everything else.
