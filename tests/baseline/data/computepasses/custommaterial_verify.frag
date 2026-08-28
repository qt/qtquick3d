// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// An Effect that runs after the main opaque/transparent render (where
// custommaterial_write.frag's CustomMaterial drew), reading back what it
// wrote. Paints magenta everywhere if too few fragments incremented the
// counter (the write path itself is broken); otherwise green where the
// marker image shows a write happened and red where it's still the
// pre-cleared "not written" sentinel (e.g. the background, outside the
// rectangle's footprint) -- proving both that the write happened AND that
// it only happened where the geometry actually rasterized.

STORAGE_BUFFER(counterBuffer, "volatile readonly") {
    uint counter[];
}

SHADER_IMAGE("rgba8", markerImage, image2D, "volatile readonly");

void MAIN()
{
    ivec2 size = imageSize(markerImage);
    ivec2 coord = clamp(ivec2(gl_FragCoord.xy), ivec2(0), size - ivec2(1));
    vec4 marker = imageLoad(markerImage, coord);

    bool writtenHere = (marker.g > 0.5 && marker.r < 0.5);
    bool countOk = (counter[0] > 1000u);

    vec3 result;
    if (!countOk)
        result = vec3(1.0, 0.0, 1.0); // magenta: too few (or zero) atomicAdds went through
    else
        result = writtenHere ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);

    FRAGCOLOR = vec4(result, 1.0);
}
