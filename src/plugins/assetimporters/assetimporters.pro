TEMPLATE = subdirs

SUBDIRS = \
    uip

qtConfig(assimp):!integrity {
    SUBDIRS += assimp
}
