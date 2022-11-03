# Generated from quick3d_visual.pro.
if(QT_BUILD_MINIMAL_STATIC_TESTS)
    add_subdirectory(smoketest)
    add_subdirectory(simplescene)
else()
    add_subdirectory(rendercontrol)
    if(NOT ANDROID) # QTBUG-102711
    add_subdirectory(multiwindow)
    endif()
    add_subdirectory(buffermanager)
    if(QT_FEATURE_private_tests)
        # QTBUG-107841
        #add_subdirectory(input)
        add_subdirectory(picking)
    endif()
endif()
