TEMPLATE = subdirs

SUBDIRS = \
    uip \
    iblbaker

qtConfig(quick3d-assimp): {
    SUBDIRS += assimp
}
