// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Fragment shader for ProceduralSkyMaterial.
// Property-to-uniform type mapping (from SkyMaterial docs):
//   color  -> vec4 (sRGB converted to linear by the engine)
//   real   -> float
//   vector3d -> vec3

const float PI = 3.14159265359;
const float HALF_PI = PI * 0.5;

float ease(float x, float c)
{
    x = clamp(x, 0.0, 1.0);
    if (c > 0.0) {
        if (c < 1.0)
            return 1.0 - pow(1.0 - x, 1.0 / c);
        return pow(x, c);
    }
    if (c < 0.0) {
        if (x < 0.5)
            return pow(x * 2.0, -c) * 0.5;
        return (1.0 - pow(1.0 - (x - 0.5) * 2.0, -c)) * 0.5 + 0.5;
    }
    return 0.0;
}

void MAIN()
{
    vec3 direction = normalize(qt_eyeDir);
    float vAngle = acos(clamp(direction.y, -1.0, 1.0));

    vec3 color;

    if (direction.y < 0.0) {
        float t = ease((vAngle - HALF_PI) / HALF_PI, groundCurve);
        color = mix(groundHorizonColor.rgb, groundBottomColor.rgb, t) * groundEnergy;
    } else {
        float t = ease(1.0 - vAngle / HALF_PI, skyCurve);
        color = mix(skyHorizonColor.rgb, skyTopColor.rgb, t) * skyEnergy;

        float sunAngle = degrees(acos(clamp(dot(normalize(sunDirection), direction), -1.0, 1.0)));
        vec3 sunRadiance = sunColor.rgb * sunEnergy;

        if (sunAngle < sunAngleMin) {
            color = sunRadiance;
        } else if (sunAngle < sunAngleMax) {
            float tSun = ease(
                (sunAngle - sunAngleMin) / (sunAngleMax - sunAngleMin),
                sunCurve
            );
            color = mix(sunRadiance, color, tSun);
        }
    }

    FRAGCOLOR = vec4(color, 1.0);
}
