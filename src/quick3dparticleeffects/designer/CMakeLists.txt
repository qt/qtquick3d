qt_copy_or_install(
    FILES
        particleeffects.metainfo
    DESTINATION "${INSTALL_QMLDIR}/QtQuick3D/ParticleEffects/designer"
)

add_subdirectory(images)
add_subdirectory(source)
