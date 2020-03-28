TEMPLATE = subdirs

SUBDIRS = \
    uip

qtConfig(quick3d-assimp):!integrity {
    SUBDIRS += assimp
}
