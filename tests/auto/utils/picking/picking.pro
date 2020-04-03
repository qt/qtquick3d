TEMPLATE = subdirs

qtConfig(private_tests) {
    SUBDIRS += intersection
}

SUBDIRS += \
    picking
