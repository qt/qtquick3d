# Generated from plugins.pro.

if (NOT INTEGRITY AND NOT WASM AND NOT QNX AND NOT rtems)
    add_subdirectory(assetimporters)
endif()
