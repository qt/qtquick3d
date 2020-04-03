TEMPLATE = subdirs

SUBDIRS = \
    uip

qtConfig(assimp):{
    SUBDIRS += assimp
}
