// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

vec3 getWorldPos(vec2 uv) {
    vec2 ndc = uv * 2.0 - 1.0;
    if (FRAMEBUFFER_Y_UP < 0.0)
        ndc.y = -ndc.y;
    float depth = texture(DEPTH_TEXTURE, uv).r;
    if (NEAR_CLIP_VALUE < 0.0)
        depth = depth * 2.0 - 1.0;
    vec4 clip = vec4(ndc, depth, 1.0);
    vec4 viewPos = INVERSE_PROJECTION_MATRIX * clip;
    viewPos /= viewPos.w;
    return (invViewMatrix * viewPos).xyz;
}

void MAIN()
{
    vec2 texcoord = TEXTURE_UV;
    if (FRAMEBUFFER_Y_UP < 0.0)
        texcoord.y = 1.0 - texcoord.y;

    vec4 sceneColor = texture(INPUT, texcoord);
    vec3 worldPos = getWorldPos(texcoord);
    float viewDistance = distance(worldPos, cameraPosition);

    float marchDistance = min(viewDistance, farPlane) - nearPlane;

    if (marchDistance <= 0.0) {
        FRAGCOLOR = sceneColor;
        return;
    }

    vec3  volumetricLight = vec3(0.0);
    float transmittance = 1.0;

    float stepSize    = marchDistance / float(marchSteps);
    float depthRange  = farPlane - nearPlane;
    float depthUVStep = stepSize / depthRange;

    vec2 screenSize = vec2(textureSize(INPUT, 0));
    vec2 noiseTexSize = textureSize(blueNoise, 0);
    vec2 noiseUV = texcoord * screenSize / noiseTexSize.xy;
    int frameOffset = int(FRAME * frameBaseJitter);

    vec2 noise = (texture(blueNoise, noiseUV + vec2(float(frameOffset % 8) / 8.,
                                                    float(frameOffset % 10) / 10.)).xy * 2.0 - 1.0) * jitterIntensity;
    vec2 jitteredTexcoord = texcoord + noise;

    for (int i = 0; (i < marchSteps && i < 128); i++) {
        float depthUV = float(i) * depthUVStep;

        vec3 froxelUV = vec3(jitteredTexcoord, depthUV);
        vec4 froxelSample = texture(froxelGrid, froxelUV);

        float extinction = exp(-froxelSample.a * stepSize);
        volumetricLight += froxelSample.rgb * transmittance * (1.0 - extinction);
        transmittance *= extinction;

        if (transmittance < 0.01)
            break;
    }

    vec3 finalColor = sceneColor.rgb * transmittance + volumetricLight;
    FRAGCOLOR = vec4(finalColor, sceneColor.a);
}
