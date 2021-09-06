# Generated from auto.pro.

#add_subdirectory(cmake) # special case
if(NOT INTEGRITY)
    add_subdirectory(quick3d)
endif()
add_subdirectory(quick3d_particles)
add_subdirectory(utils)
add_subdirectory(tools)
add_subdirectory(quick3d_visual)
if((android_app OR NOT ANDROID) AND (android_app OR NOT INTEGRITY) AND (NOT ANDROID OR NOT CMAKE_CROSSCOMPILING) AND (NOT ANDROID OR NOT WASM) AND (NOT CMAKE_CROSSCOMPILING OR NOT INTEGRITY) AND (NOT INTEGRITY OR NOT WASM))
    add_subdirectory(assetimport)
endif()
