# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(TARGET WrapOpenXR::WrapOpenXR)
    set(WrapOpenXR_FOUND TRUE)
    return()
endif()
set(WrapOpenXR_FOUND FALSE)

if(ANDROID)
    if (NOT "$ENV{OCULUS_OPENXR_MOBILE_SDK}" STREQUAL "")
        add_library(openxr_loader SHARED IMPORTED)
        set(XR_LOADER_TYPE "Debug")
        if(DEFINED CMAKE_BUILD_TYPE)
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                set(XR_LOADER_TYPE "Debug")
            elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
                set(XR_LOADER_TYPE "Release")
            elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
                set(XR_LOADER_TYPE "Release")
            endif()
        endif()
        file(TO_CMAKE_PATH $ENV{OCULUS_OPENXR_MOBILE_SDK} OCULUS_OPENXR_MOBILE_SDK)
        set(XR_LOADER_SO ${OCULUS_OPENXR_MOBILE_SDK}/OpenXR/Libs/Android/${ANDROID_ABI}/${XR_LOADER_TYPE}/libopenxr_loader.so)
        message("Using OpenXR loader ${XR_LOADER_SO}")
        set_property(
            TARGET
                openxr_loader
            PROPERTY
                IMPORTED_LOCATION
            ${XR_LOADER_SO}
        )
        add_library(WrapOpenXR::WrapOpenXR INTERFACE IMPORTED)
        target_link_libraries(WrapOpenXR::WrapOpenXR INTERFACE openxr_loader)
        target_include_directories(
            WrapOpenXR::WrapOpenXR INTERFACE
            ${OCULUS_OPENXR_MOBILE_SDK}/OpenXR/Include
            ${OCULUS_OPENXR_MOBILE_SDK}/3rdParty/khronos/openxr/OpenXR-SDK/include
        )
        set(WrapOpenXR_FOUND TRUE)
    else()
          message("OCULUS_OPENXR_MOBILE_SDK is not set")
    endif()
else()
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.27.0")
        cmake_policy(SET CMP0144 NEW)
    endif()
    find_package(OpenXR ${WrapOpenXR_FIND_VERSION} QUIET)
    if (OpenXR_FOUND AND TARGET OpenXR::openxr_loader)
        # get rid of Threads::Threads
        if(WIN32)
              set_target_properties(OpenXR::openxr_loader PROPERTIES INTERFACE_LINK_LIBRARIES "OpenXR::headers;advapi32")
        else()
            set_target_properties(OpenXR::openxr_loader PROPERTIES INTERFACE_LINK_LIBRARIES "OpenXR::headers")
        endif()
        add_library(WrapOpenXR::WrapOpenXR INTERFACE IMPORTED)
        target_link_libraries(WrapOpenXR::WrapOpenXR INTERFACE OpenXR::openxr_loader)
        set(WrapOpenXR_FOUND TRUE)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapOpenXR DEFAULT_MSG WrapOpenXR_FOUND)
