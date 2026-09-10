// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// arrayMap is a sampler2DArray that nothing binds, so it has to end up with
// the engine's dummy texture array, which is opaque black. Sampling it here
// also keeps the sampler live, so it cannot be optimized out of the shader's
// resource list and stop being a binding the pass has to fill at all.

void MAIN()
{
    // Both layers are read: the dummy has to be an array deep enough to be
    // indexed, not a single-layer stand-in.
    vec4 layer0 = texture(arrayMap, vec3(0.5, 0.5, 0.0));
    vec4 layer1 = texture(arrayMap, vec3(0.5, 0.5, 1.0));

    bool ok = layer0.r < 0.05 && layer0.g < 0.05 && layer0.b < 0.05 && layer0.a > 0.95
           && layer1.r < 0.05 && layer1.g < 0.05 && layer1.b < 0.05 && layer1.a > 0.95;

    // The augment shader puts this on screen as it is, so lighting never
    // enters into it.
    BASE_COLOR = ok ? vec4(0.0, 1.0, 0.0, 1.0) : vec4(1.0, 0.0, 0.0, 1.0);
}
