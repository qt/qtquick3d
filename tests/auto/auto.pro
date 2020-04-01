TEMPLATE = subdirs
SUBDIRS = \
    cmake \
    quick3d \
    utils

!integrity:!android|android_app:!wasm:!cross_compile {
    SUBDIRS += \
        assetimport \
        quick3d_lancelot
}
