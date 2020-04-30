TEMPLATE = subdirs

SUBDIRS = \
    uip

qtConfig(quick3d-assimp): {
    SUBDIRS += assimp
}
