qt_internal_add_3rdparty_library(BundledEmbree
    STATIC
    INSTALL
    EXCEPTIONS
)

qt_internal_extend_target(BundledEmbree
    SOURCES
        common/sys/sysinfo.cpp
        common/sys/alloc.cpp
        common/sys/filename.cpp
        common/sys/library.cpp
        common/sys/thread.cpp
        common/sys/string.cpp
        common/sys/regression.cpp
        common/sys/mutex.cpp
        common/sys/condition.cpp
        common/sys/barrier.cpp
        common/math/constants.cpp
        common/simd/sse.cpp
        common/lexers/stringstream.cpp
        common/lexers/tokenstream.cpp
        common/tasking/taskschedulerinternal.cpp
        kernels/common/device.cpp
        kernels/common/stat.cpp
        kernels/common/acceln.cpp
        kernels/common/accelset.cpp
        kernels/common/state.cpp
        kernels/common/rtcore.cpp
        kernels/common/rtcore_builder.cpp
        kernels/common/scene.cpp
        kernels/common/alloc.cpp
        kernels/common/geometry.cpp
        kernels/common/scene_triangle_mesh.cpp
        kernels/geometry/primitive4.cpp
        kernels/builders/primrefgen.cpp
        kernels/bvh/bvh.cpp
        kernels/bvh/bvh_statistics.cpp
        kernels/bvh/bvh4_factory.cpp
        kernels/bvh/bvh8_factory.cpp
        kernels/bvh/bvh_collider.cpp
        kernels/bvh/bvh_rotate.cpp
        kernels/bvh/bvh_refit.cpp
        kernels/bvh/bvh_builder.cpp
        kernels/bvh/bvh_builder_morton.cpp
        kernels/bvh/bvh_builder_sah.cpp
        kernels/bvh/bvh_builder_sah_spatial.cpp
        kernels/bvh/bvh_builder_sah_mb.cpp
        kernels/bvh/bvh_builder_twolevel.cpp
        kernels/bvh/bvh_intersector1_bvh4.cpp
)

qt_disable_warnings(BundledEmbree)
qt_set_symbol_visibility_hidden(BundledEmbree)

if (WIN32)
    qt_disable_unicode_defines(BundledEmbree)
endif()

if (IOS)
    qt_internal_extend_target(BundledEmbree COMPILE_OPTIONS -fembed-bitcode)
endif()

# Use SSE2 only, ignore AVX/SSE4.2 for now
qt_internal_extend_target(BundledEmbree DEFINES
    EMBREE_TARGET_SSE2
    __SSE2__
    __SSE__
    EMBREE_LOWEST_ISA
    TASKING_INTERNAL
    NDEBUG
)
