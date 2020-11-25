TEMPLATE = subdirs
SUBDIRS = \
    cmake \
    quick3d \
    utils \
    tools \
    quick3d_visual

!integrity:!android|android_app:!wasm:!cross_compile {
    SUBDIRS += \
        assetimport \
        quick3d_lancelot
}
