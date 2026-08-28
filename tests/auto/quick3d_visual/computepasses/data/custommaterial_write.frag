// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A CustomMaterial (as opposed to effect_write.frag's Effect) writing to a
// ShaderStorageBuffer and a ShaderImage. Rendered on real geometry, so unlike
// a full-screen Effect, the number of invocations isn't exactly known in
// advance -- only that it's however many fragments the geometry covers.
// gl_FragCoord.xy (not TEXTURE_UV, which is Effect-only) addresses the
// marker image, using the same window-space coordinate that
// custommaterial_verify.frag reads back with, so the two agree regardless
// of any backend Y-convention.
//
// CAVEAT: writing to a storage buffer or image from a graphics (vertex/
// fragment) pipeline stage, as opposed to compute, depends on the graphics
// API: Direct3D 11 only allows it in fragment shaders, and OpenGL ES 3.1
// implementations are not required to support it at all. Where it is
// supported, QRhi synchronizes storage buffers and storage images across
// passes alike. A failure here may therefore indicate a backend limitation
// rather than an engine bug.

STORAGE_BUFFER(counterBuffer, "coherent restrict") {
    uint counter[];
}

SHADER_IMAGE("rgba8", markerImage, image2D, "writeonly");

void MAIN()
{
    atomicAdd(counter[0], 1u);

    ivec2 size = imageSize(markerImage);
    ivec2 coord = clamp(ivec2(gl_FragCoord.xy), ivec2(0), size - ivec2(1));
    imageStore(markerImage, coord, vec4(0.0, 1.0, 0.0, 1.0));

    FRAGCOLOR = vec4(1.0, 1.0, 1.0, 1.0);
}
