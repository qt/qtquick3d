// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Checks that the augmented pass binds the right texture to each sampler it
// declares. The samplers come from Texture properties of the RenderPass, but
// only one of them resolves to a GPU texture; the others stay unbound and have
// to receive the engine's dummy texture, which is opaque black.
//
// Sampling the unbound ones is deliberate: it keeps the samplers live so they
// cannot be optimized out of the shader's resource list, and it fails the
// check if the dummy fill were ever to overwrite the bound texture instead.

void MAIN_FRAGMENT_AUGMENT()
{
    // The source rectangles are a single flat colour, so a fixed coordinate is
    // enough and this does not depend on the model's UVs.
    vec4 fromBound = texture(boundMap, vec2(0.5));
    vec4 fromUnbound = texture(unboundMap, vec2(0.5));

    // The dummy cube map is uploaded face by face. Only +X used to be filled,
    // leaving the rest undefined, so sample three of the other five. Requiring
    // opaque black rather than just black is what catches an unwritten face:
    // uninitialized memory reads back with a zero alpha far more often than
    // with a one.
    vec4 cubeNegX = texture(unboundCube, vec3(-1.0, 0.0, 0.0));
    vec4 cubePosY = texture(unboundCube, vec3(0.0, 1.0, 0.0));
    vec4 cubeNegZ = texture(unboundCube, vec3(0.0, 0.0, -1.0));

    bool boundOk = fromBound.r < 0.25 && fromBound.g > 0.75 && fromBound.b < 0.25;
    bool unboundOk = fromUnbound.r < 0.05 && fromUnbound.g < 0.05
                  && fromUnbound.b < 0.05 && fromUnbound.a > 0.95;
    bool cubeOk = cubeNegX.r < 0.05 && cubeNegX.g < 0.05 && cubeNegX.b < 0.05 && cubeNegX.a > 0.95
               && cubePosY.r < 0.05 && cubePosY.g < 0.05 && cubePosY.b < 0.05 && cubePosY.a > 0.95
               && cubeNegZ.r < 0.05 && cubeNegZ.g < 0.05 && cubeNegZ.b < 0.05 && cubeNegZ.a > 0.95;

    fragOutput = (boundOk && unboundOk && cubeOk) ? vec4(0.0, 1.0, 0.0, 1.0) : vec4(1.0, 0.0, 0.0, 1.0);
}
