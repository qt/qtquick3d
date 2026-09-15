// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

SHADER_IMAGE("rgba32f", froxelImage, image3D, "readonly");

vec3 getWorldPos(vec2 uv, out vec3 cameraPos) {
    vec2 ndc = uv * 2.0 - 1.0;
    if (FRAMEBUFFER_Y_UP < 0.0)
        ndc.y = -ndc.y;
    float depth = texture(DEPTH_TEXTURE, uv).r;
    if (NEAR_CLIP_VALUE < 0.0)
        depth = depth * 2.0 - 1.0;
    vec4 clip = vec4(ndc, depth, 1.0);
    vec4 viewPos = INVERSE_PROJECTION_MATRIX * clip;
    viewPos /= viewPos.w;
    mat4 iv = inverse(VIEW_MATRIX);
    cameraPos = vec3(iv[3]);
    return (iv * viewPos).xyz;
}

void MAIN()
{
    float nearPlane = CAMERA_PROPERTIES.x;
    float farPlane  = CAMERA_PROPERTIES.y;
    vec2 texcoord = TEXTURE_UV;
    if (FRAMEBUFFER_Y_UP < 0.0)
        texcoord.y = 1.0 - texcoord.y;

    vec3 cameraPosition;

    vec4 sceneColor = texture(INPUT, texcoord);
    vec3 worldPos = getWorldPos(texcoord, cameraPosition);
    float viewDistance = distance(worldPos, cameraPosition);

    float marchDistance = min(viewDistance, farPlane) - nearPlane;

    if (marchDistance <= 0.0) {
        FRAGCOLOR = sceneColor;
        return;
    }

    vec3  volumetricLight = vec3(0.0);
    float transmittance = 1.0;

    vec3  fwd  = -vec3(VIEW_MATRIX[0][2], VIEW_MATRIX[1][2], VIEW_MATRIX[2][2]);
    float cosT = dot(normalize(worldPos - cameraPosition), fwd);

    float stepSize    = marchDistance / float(64);
    float depthRange  = farPlane - nearPlane;
    // slices are spaced by planar view depth, not radial distance
    float depthUVStep = stepSize * cosT / depthRange;

    vec2 screenSize = vec2(textureSize(INPUT, 0));
    vec2 noiseTexSize = textureSize(blueNoise, 0);
    vec2 noiseUV = texcoord * screenSize / noiseTexSize.xy;
    vec2 noise = (texture(blueNoise, noiseUV).xy * 2.0 - 1.0) * jitterIntensity;
    vec2 jitteredTexcoord = texcoord + noise;

    for (int i = 0; i < 64; i++) {
        float depthUV = (float(i) + 0.5) * depthUVStep;

        vec3 froxelUV = clamp(vec3(jitteredTexcoord, depthUV), 0.0, 1.0);
        vec4 froxelSample = imageLoad(froxelImage, ivec3(froxelUV * (gridDimensions - 1.0)));

        float extinction = exp(-froxelSample.a * stepSize);
        volumetricLight += froxelSample.rgb * transmittance * (1.0 - extinction);
        transmittance *= extinction;

        if (transmittance < 0.01)
            break;
    }

    vec3 finalColor = sceneColor.rgb * transmittance + volumetricLight;
    FRAGCOLOR = vec4(finalColor, sceneColor.a);
}
