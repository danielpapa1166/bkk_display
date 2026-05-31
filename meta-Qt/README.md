# meta-Qt

This layer integrates Qt5 into the bkk_display Yocto build (kirkstone).

It exists to isolate all Qt-related configuration from the platform and application layers, keeping Qt customizations in one place.

## What it provides

- `qtbase` configured with embedded platform plugins: `eglfs`, `kms`, `gbm`, `linuxfb`
  (no X11 required — suitable for bare-metal display targets)
- Image extension that installs `qtbase`, `liberation-fonts`, `bkk-qt-app`, and `bkk-helper` into the final image

## Dependencies

- `meta` (OE-core, kirkstone)
- `meta-qt5` (provides the `qtbase` recipe being appended)
