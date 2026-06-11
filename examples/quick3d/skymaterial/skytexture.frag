// Copyright (C) 2023 Fernando García Liñán
// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: MIT

// Configurable parameters

// Ray marching steps. More steps mean better accuracy but worse performance
const int TRANSMITTANCE_STEPS     = 32;
const int IN_SCATTERING_STEPS     = 16;

// Debug
#define ENABLE_SPECTRAL 1
#define ENABLE_MULTIPLE_SCATTERING 1
#define ENABLE_AEROSOLS 1

//-----------------------------------------------------------------------------
// Constants

// All parameters that depend on wavelength (vec4) are sampled at
// 630, 560, 490, 430 nanometers

const float PI = 3.14159265358979323846;
const float INV_PI = 0.31830988618379067154;
const float INV_4PI = 0.25 * INV_PI;
const float PHASE_ISOTROPIC = INV_4PI;
const float RAYLEIGH_PHASE_SCALE = (3.0 / 16.0) * INV_PI;
const float g = 0.8;
const float gg = g*g;

const float EARTH_RADIUS = 6371.0; // km
const float ATMOSPHERE_THICKNESS = 100.0; // km
const float ATMOSPHERE_RADIUS = EARTH_RADIUS + ATMOSPHERE_THICKNESS;

#if ENABLE_SPECTRAL == 1
// Extraterrestial Solar Irradiance Spectra, units W * m^-2 * nm^-1
// https://www.nrel.gov/grid/solar-resource/spectra.html
const vec4 sunSpectralIrradiance = vec4(1.679, 1.828, 1.986, 1.307);
// Rayleigh scattering coefficient at sea level, units km^-1
// "Rayleigh-scattering calculations for the terrestrial atmosphere"
// by Anthony Bucholtz (1995).
const vec4 molecularScatteringCoefficientBase = vec4(6.605e-3, 1.067e-2, 1.842e-2, 3.156e-2);
// Ozone absorption cross section, units m^2 / molecules
// "High spectral resolution ozone absorption cross-sections"
// by V. Gorshelev et al. (2014).
const vec4 ozoneAbsorptionCrossSection = vec4(3.472e-21, 3.914e-21, 1.349e-21, 11.03e-23) * 1e-4f;
#else
// Same as above but for the following "RGB" wavelengths: 680, 550, 440 nm
// The Sun spectral irradiance is also multiplied by a constant factor to
// compensate for the fact that we use the spectral samples directly as RGB,
// which is incorrect.
const vec4 sunSpectralIrradiance = vec4(1.500, 1.864, 1.715, 0.0) * 150.0;
const vec4 molecularScatteringCoefficientBase = vec4(4.847e-3, 1.149e-2, 2.870e-2, 0.0);
const vec4 ozoneAbsorptionCrossSection = vec4(3.36e-21f, 3.08e-21f, 20.6e-23f, 0.0) * 1e-4f;
#endif

/*
 * Every aerosol type expects 5 parameters:
 * - Scattering cross section
 * - Absorption cross section
 * - Base density (km^-3)
 * - Background density (km^-3)
 * - Height scaling parameter
 * These parameters can be sent as uniforms.
 *
 * This model for aerosols and their corresponding parameters come from
 * "A Physically-Based Spatio-Temporal Sky Model"
 * by Guimera et al. (2018).
 */

vec3 getSunDirection(float time)
{
    float a = sin(time*0.5 - 1.5) * 0.55 + 0.45;
    return vec3(-sqrt(1.0 - a*a), 0.0, a);
}

/*
 * Helper function to obtain the transmittance to the top of the atmosphere
 * from Buffer A.
 */
vec4 transmittanceFromLut(sampler2D lut, float cosTheta, float normalizedAltitude)
{
    float u = clamp(cosTheta * 0.5 + 0.5, 0.0, 1.0);
    float v = clamp(normalizedAltitude, 0.0, 1.0);
    return texture(lut, vec2(u, v));
}

/*
 * Returns the distance between ro and the first intersection with the sphere
 * or -1.0 if there is no intersection. The sphere's origin is (0,0,0).
 * -1.0 is also returned if the ray is pointing away from the sphere.
 */
float raySphereIntersection(vec3 ro, vec3 rd, float radius)
{
    float b = dot(ro, rd);
    float c = dot(ro, ro) - radius*radius;
    if (c > 0.0 && b > 0.0) return -1.0;
    float d = b*b - c;
    if (d < 0.0) return -1.0;
    if (d > b*b) return (-b+sqrt(d));
    return (-b-sqrt(d));
}

/*
 * Rayleigh phase function.
 */
float molecularPhaseFunction(float cosTheta)
{
    return RAYLEIGH_PHASE_SCALE * (1.0 + cosTheta*cosTheta);
}

/*
 * Henyey-Greenstrein phase function.
 */
float aerosolPhaseFunction(float cosTheta)
{
    float den = 1.0 + gg + 2.0 * g * cosTheta;
    return INV_4PI * (1.0 - gg) / (den * sqrt(den));
}

vec4 getMultipleScattering(sampler2D transmittanceLut, float cosTheta, float normalizedHeight, float d, vec4 groundAlbedo)
{
#if ENABLE_MULTIPLE_SCATTERING == 1
    // Solid angle subtended by the planet from a point at d distance
    // from the planet center.
    float omega = 2.0 * PI * (1.0 - sqrt(d*d - EARTH_RADIUS*EARTH_RADIUS) / d);

    vec4 tToGround = transmittanceFromLut(transmittanceLut, cosTheta, 0.0);

    vec4 tGroundToSample =
        transmittanceFromLut(transmittanceLut, 1.0, 0.0) /
        transmittanceFromLut(transmittanceLut, 1.0, normalizedHeight);

    // 2nd order scattering from the ground
    vec4 lGround = PHASE_ISOTROPIC * omega * (groundAlbedo / PI) * tToGround * tGroundToSample * cosTheta;

    // Fit of Earth's multiple scattering coming from other points in the atmosphere
    vec4 lMs = 0.02 * vec4(0.217, 0.347, 0.594, 1.0) * (1.0 / (1.0 + 5.0 * exp(-17.92 * cosTheta)));

    return lMs + lGround;
#else
    return vec4(0.0);
#endif
}

/*
 * Return the molecular volume scattering coefficient (km^-1) for a given altitude
 * in kilometers.
 */
vec4 getMolecularScatteringCoefficient(float h)
{
    return molecularScatteringCoefficientBase * exp(-0.07771971 * pow(h, 1.16364243));
}

/*
 * Return the molecular volume absorption coefficient (km^-1) for a given altitude
 * in kilometers.
 */
vec4 getMolecularAbsorptionCoefficient(float h, float ozoneMeanMonthlyDobson)
{
    h += 1e-4; // Avoid division by 0
    float t = log(h) - 3.22261;
    float density = 3.78547397e20 * (1.0 / h) * exp(-t * t * 5.55555555);
    return ozoneAbsorptionCrossSection * ozoneMeanMonthlyDobson * density;
}

float getAerosolDensity(float h,
                        int aerosolType,
                        float aerosolBaseDensity,
                        float aerosolHeightScale,
                        float aerosolBackgroundDividedByBaseDensity
                        )
{
    if (aerosolType == 0) { // Only for the Background aerosol type, no dependency on height
        return aerosolBaseDensity * (1.0 + aerosolBackgroundDividedByBaseDensity);
    } else {
        return aerosolBaseDensity * (exp(-h / aerosolHeightScale)
            + aerosolBackgroundDividedByBaseDensity);
    }
}

/*
 * Get the collision coefficients (scattering and absorption) of the
 * atmospheric medium for a given point at an altitude h.
 */
void getAtmosphereCollisionCoefficients(in float h,
                                        in vec4 aerosolAbsorptionCrossSection,
                                        in vec4 aerosolScatteringCrossSection,
                                        in float aerosolBaseDensity,
                                        in float aerosolHeightScale,
                                        in float aerosolBackgroundDividedByBaseDensity,
                                        in int aerosolType,
                                        in float ozoneMeanMonthlyDobson,
                                        in float aerosolTurbidity,
                                        out vec4 aerosolAbsorption,
                                        out vec4 aerosolScattering,
                                        out vec4 molecularAbsorption,
                                        out vec4 molecularScattering,
                                        out vec4 extinction)
{
    h = max(h, 0.0); // In case height is negative
#if ENABLE_AEROSOLS == 0
    aerosolAbsorption = vec4(0.0);
    aerosolScattering = vec4(0.0);
#else
    float aerosolDensity = getAerosolDensity(h, aerosolType, aerosolBaseDensity, aerosolHeightScale, aerosolBackgroundDividedByBaseDensity);
    aerosolAbsorption = aerosolAbsorptionCrossSection * aerosolDensity * aerosolTurbidity;
    aerosolScattering = aerosolScatteringCrossSection * aerosolDensity * aerosolTurbidity;
#endif
    molecularAbsorption = getMolecularAbsorptionCoefficient(h, ozoneMeanMonthlyDobson);
    molecularScattering = getMolecularScatteringCoefficient(h);
    extinction = aerosolAbsorption + aerosolScattering + molecularAbsorption + molecularScattering;
}

//-----------------------------------------------------------------------------
// Spectral rendering stuff

const mat4 M = mat4(
    137.672389239975, -8.632904716299537, -1.7181567391931372, 0,
    32.549094028629234, 91.29801417199785, -12.005406444382531, 0,
    -38.91428392614275, 34.31665471469816, 29.89044807197628, 0,
    8.572844237945445, -11.103384660054624, 117.47585277566478, 0
);

vec3 linearSrgbFromSpectralSamples(vec4 L)
{
    return (M * L).xyz;
}

/*
 * Buffer B: Sky texture
 *
 * "A Scalable and Production Ready Sky and Atmosphere Rendering Technique"
 * by Sébastien Hillaire (2020).
 *
 * We render the sky to a texture instead of raymarching on the entire screen.
 * This is not very useful in Shadertoy, but very useful for someone looking
 * to implement this on a real application.
 *
 * It is important to note that quality decreases significantly when rendering
 * space views. To avoid this, the computeInscattering() function can be used
 * directly when rendering to a fullscreen quad.
 */

vec4 computeInscattering(vec3 rayOrigin, vec3 rayDir, float tD, out vec4 transmittance)
{

    float cosTheta = dot(-rayDir, sunDirection);

    float molecularPhase = molecularPhaseFunction(cosTheta);
    float aerosolPhase = aerosolPhaseFunction(cosTheta);

    float dt = tD / float(IN_SCATTERING_STEPS);

    vec4 lInscattering = vec4(0.0);
    transmittance = vec4(1.0);

    for (int i = 0; i < IN_SCATTERING_STEPS; ++i) {
        float t = (float(i) + 0.5) * dt;
        vec3 xT = rayOrigin + rayDir * t;

        float distanceToEarthCenter = length(xT);
        vec3 zenithDir = xT / distanceToEarthCenter;
        float altitude = distanceToEarthCenter - EARTH_RADIUS;
        float normalizedAltitude = altitude / ATMOSPHERE_THICKNESS;

        float sampleCosTheta = dot(zenithDir, sunDirection);

        vec4 aerosolAbsorption, aerosolScattering;
        vec4 molecularAbsorption, molecularScattering;
        vec4 extinction;
        getAtmosphereCollisionCoefficients(
            altitude,
            aerosolAbsorptionCrossSection,
            aerosolScatteringCrossSection,
            aerosolBaseDensity,
            aerosolHeightScale,
            aerosolBackgroundDividedByBaseDensity,
            aerosolType,
            ozoneMeanMonthlyDobson,
            aerosolTurbidity,
            aerosolAbsorption, aerosolScattering,
            molecularAbsorption, molecularScattering,
            extinction);

        vec4 transmittanceToSun = transmittanceFromLut(
            transmittanceBuffer, sampleCosTheta, normalizedAltitude);

        vec4 ms = getMultipleScattering(
            transmittanceBuffer, sampleCosTheta, normalizedAltitude,
            distanceToEarthCenter,
            groundAlbedo);

        vec4 S = sunSpectralIrradiance *
            (molecularScattering * (molecularPhase * transmittanceToSun + ms) +
             aerosolScattering   * (aerosolPhase   * transmittanceToSun + ms));

        vec4 stepTransmittance = exp(-dt * extinction);

        // Energy-conserving analytical integration
        // "Physically Based Sky, Atmosphere and Cloud Rendering in Frostbite"
        // by Sébastien Hillaire
        vec4 sInt = (S - S * stepTransmittance) / max(extinction, 1e-7);
        lInscattering += transmittance * sInt;
        transmittance *= stepTransmittance;
    }

    return lInscattering;
}

void MAIN()
{
    vec2 uv = INPUT_UV;
    const float EYE_DISTANCE_TO_EARTH_CENTER = EARTH_RADIUS + eyeAltitude;

    float azimuth = 2.0 * PI * uv.x;

    // Apply a non-linear transformation to the elevation to dedicate more
    // texels to the horizon, where having more detail matters.
    float l = uv.y * 2.0 - 1.0;
    float elev = l*l * sign(l) * PI * 0.5; // [-pi/2, pi/2]

    vec3 rayDir = vec3(cos(elev) * cos(azimuth),
                        cos(elev) * sin(azimuth),
                        sin(elev));

    vec3 rayOrigin = vec3(0.0, 0.0, EYE_DISTANCE_TO_EARTH_CENTER);

    float atmosDist  = raySphereIntersection(rayOrigin, rayDir, ATMOSPHERE_RADIUS);
    float groundDist = raySphereIntersection(rayOrigin, rayDir, EARTH_RADIUS);
    float tD;
    if (eyeAltitude < ATMOSPHERE_THICKNESS) {
        // We are inside the atmosphere
        if (groundDist < 0.0) {
            // No ground collision, use the distance to the outer atmosphere
            tD = atmosDist;
        } else {
            // We have a collision with the ground, use the distance to it
            tD = groundDist;
        }
    } else {
        // We are in outer space
        if (atmosDist < 0.0) {
            // No collision with the atmosphere, just return black
            FRAGCOLOR = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        } else {
            // Move the ray origin to the atmosphere intersection
            rayOrigin = rayOrigin + rayDir * (atmosDist + 1e-3);
            if (groundDist < 0.0) {
                // No collision with the ground, so the ray is exiting through
                // the atmosphere.
                float secondAtmosDist = raySphereIntersection(
                    rayOrigin, rayDir, ATMOSPHERE_RADIUS);
                tD = secondAtmosDist;
            } else {
                tD = groundDist - atmosDist;
            }
        }
    }

    vec4 transmittance;
    vec4 L = computeInscattering(rayOrigin, rayDir, tD, transmittance);

#if ENABLE_SPECTRAL == 1
    FRAGCOLOR = vec4(linearSrgbFromSpectralSamples(L), 1.0);
#else
    FRAGCOLOR = vec4(L.rgb, 1.0);
#endif


}
