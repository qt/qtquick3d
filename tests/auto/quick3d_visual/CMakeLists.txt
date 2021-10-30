# Generated from quick3d_visual.pro.

add_subdirectory(smoketest)
add_subdirectory(simplescene)
add_subdirectory(rendercontrol)
add_subdirectory(multiwindow)
if(QT_FEATURE_private_tests)
    add_subdirectory(input)
    add_subdirectory(picking)
endif()

