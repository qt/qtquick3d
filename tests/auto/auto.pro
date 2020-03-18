TEMPLATE = subdirs
SUBDIRS = \
    cmake \
    quick3d \
    quick3d_lancelot \
    utils

!integrity:!android|android_app:!wasm:!cross_compile {
    SUBDIRS += assetimport
}
