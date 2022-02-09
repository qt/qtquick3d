# Generated from quick3d_visual.pro.

if(QT_BUILD_MINIMAL_STATIC_TESTS)
    add_subdirectory(smoketest)
    add_subdirectory(simplescene)
else()
    add_subdirectory(rendercontrol)
    add_subdirectory(multiwindow)
    if(QT_FEATURE_private_tests)
        add_subdirectory(input)
        add_subdirectory(picking)
    endif()
endif()

