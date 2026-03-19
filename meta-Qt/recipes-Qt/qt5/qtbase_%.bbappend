# Enable direct rendering platform plugins for embedded targets (no X11)
PACKAGECONFIG:append = " eglfs kms gbm linuxfb"
