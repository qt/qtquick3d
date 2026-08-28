// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// An Effect (not a ComputePass) writing to a ShaderStorageBuffer and a
// ShaderImage. Every prior test in this suite only ever reads these
// resources from a material/effect (via imageLoad); this is the first to
// write from one, using the exact same binding-type derivation
// (readonly/writeonly/else -> Load/Store/LoadStore) qssgrhieffectsystem.cpp
// shares with qssgcomputemanager.cpp and qssgrhicustommaterialsystem.cpp.
//
// Runs once per output pixel (guaranteed by the Effect pipeline, unlike a
// CustomMaterial on real geometry where rasterization coverage isn't exactly
// known), so the buffer counter reaches an exact, predictable total.
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
    vec2 texcoord = TEXTURE_UV;
    if (FRAMEBUFFER_Y_UP < 0.0)
        texcoord.y = 1.0 - texcoord.y;
    ivec2 coord = clamp(ivec2(texcoord * vec2(size)), ivec2(0), size - ivec2(1));
    imageStore(markerImage, coord, vec4(0.0, 1.0, 0.0, 1.0));

    FRAGCOLOR = texture(INPUT, texcoord);
}
