// Copyright (C) 2023 Fernando García Liñán
// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: MIT

// Configurable parameters

// Ray marching steps. More steps mean better accuracy but worse performance
const int TRANSMITTANCE_STEPS     = 32;

#define ENABLE_SPECTRAL 1
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
const vec4 sun_spectral_irradiance = vec4(1.679, 1.828, 1.986, 1.307);
// Rayleigh scattering coefficient at sea level, units km^-1
// "Rayleigh-scattering calculations for the terrestrial atmosphere"
// by Anthony Bucholtz (1995).
const vec4 molecular_scattering_coefficient_base = vec4(6.605e-3, 1.067e-2, 1.842e-2, 3.156e-2);
// Ozone absorption cross section, units m^2 / molecules
// "High spectral resolution ozone absorption cross-sections"
// by V. Gorshelev et al. (2014).
const vec4 ozone_absorption_cross_section = vec4(3.472e-21, 3.914e-21, 1.349e-21, 11.03e-23) * 1e-4f;
#else
// Same as above but for the following "RGB" wavelengths: 680, 550, 440 nm
// The Sun spectral irradiance is also multiplied by a constant factor to
// compensate for the fact that we use the spectral samples directly as RGB,
// which is incorrect.
const vec4 sun_spectral_irradiance = vec4(1.500, 1.864, 1.715, 0.0) * 150.0;
const vec4 molecular_scattering_coefficient_base = vec4(4.847e-3, 1.149e-2, 2.870e-2, 0.0);
const vec4 ozone_absorption_cross_section = vec4(3.36e-21f, 3.08e-21f, 20.6e-23f, 0.0) * 1e-4f;
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

vec3 get_sun_direction(float time)
{
    float a = sin(time*0.5 - 1.5) * 0.55 + 0.45;
    return vec3(-sqrt(1.0 - a*a), 0.0, a);
}

/*
 * Helper function to obtain the transmittance to the top of the atmosphere
 * from Buffer A.
 */
vec4 transmittance_from_lut(sampler2D lut, float cos_theta, float normalized_altitude)
{
    float u = clamp(cos_theta * 0.5 + 0.5, 0.0, 1.0);
    float v = clamp(normalized_altitude, 0.0, 1.0);
    return texture(lut, vec2(u, v));
}

/*
 * Returns the distance between ro and the first intersection with the sphere
 * or -1.0 if there is no intersection. The sphere's origin is (0,0,0).
 * -1.0 is also returned if the ray is pointing away from the sphere.
 */
float ray_sphere_intersection(vec3 ro, vec3 rd, float radius)
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
float molecular_phase_function(float cos_theta)
{
    return RAYLEIGH_PHASE_SCALE * (1.0 + cos_theta*cos_theta);
}

/*
 * Henyey-Greenstrein phase function.
 */
float aerosol_phase_function(float cos_theta)
{
    float den = 1.0 + gg + 2.0 * g * cos_theta;
    return INV_4PI * (1.0 - gg) / (den * sqrt(den));
}

vec4 get_multiple_scattering(sampler2D transmittance_lut, float cos_theta, float normalized_height, float d, vec4 ground_albedo)
{
#if ENABLE_MULTIPLE_SCATTERING == 1
    // Solid angle subtended by the planet from a point at d distance
    // from the planet center.
    float omega = 2.0 * PI * (1.0 - sqrt(d*d - EARTH_RADIUS*EARTH_RADIUS) / d);

    vec4 T_to_ground = transmittance_from_lut(transmittance_lut, cos_theta, 0.0);

    vec4 T_ground_to_sample =
        transmittance_from_lut(transmittance_lut, 1.0, 0.0) /
        transmittance_from_lut(transmittance_lut, 1.0, normalized_height);

    // 2nd order scattering from the ground
    vec4 L_ground = PHASE_ISOTROPIC * omega * (ground_albedo / PI) * T_to_ground * T_ground_to_sample * cos_theta;

    // Fit of Earth's multiple scattering coming from other points in the atmosphere
    vec4 L_ms = 0.02 * vec4(0.217, 0.347, 0.594, 1.0) * (1.0 / (1.0 + 5.0 * exp(-17.92 * cos_theta)));

    return L_ms + L_ground;
#else
    return vec4(0.0);
#endif
}

/*
 * Return the molecular volume scattering coefficient (km^-1) for a given altitude
 * in kilometers.
 */
vec4 get_molecular_scattering_coefficient(float h)
{
    return molecular_scattering_coefficient_base * exp(-0.07771971 * pow(h, 1.16364243));
}

/*
 * Return the molecular volume absorption coefficient (km^-1) for a given altitude
 * in kilometers.
 */
vec4 get_molecular_absorption_coefficient(float h, float ozone_mean_monthly_dobson)
{
    h += 1e-4; // Avoid division by 0
    float t = log(h) - 3.22261;
    float density = 3.78547397e20 * (1.0 / h) * exp(-t * t * 5.55555555);
    return ozone_absorption_cross_section * ozone_mean_monthly_dobson * density;
}

float get_aerosol_density(float h,
                          int aerosol_type,
                          float aerosol_base_density,
                          float aerosol_height_scale,
                          float aerosol_background_divided_by_base_density
                          )
{
    if (aerosol_type == 0) { // Only for the Background aerosol type, no dependency on height
        return aerosol_base_density * (1.0 + aerosol_background_divided_by_base_density);
    } else {
        return aerosol_base_density * (exp(-h / aerosol_height_scale)
            + aerosol_background_divided_by_base_density);
    }
}

/*
 * Get the collision coefficients (scattering and absorption) of the
 * atmospheric medium for a given point at an altitude h.
 */
void get_atmosphere_collision_coefficients(in float h,
                                           in vec4 aerosol_absorption_cross_section,
                                           in vec4 aerosol_scattering_cross_section,
                                           in float aerosol_base_density,
                                           in float aerosol_height_scale,
                                           in float aerosol_background_divided_by_base_density,
                                           in int aerosol_type,
                                           in float ozone_mean_monthly_dobson,
                                           in float aerosol_turbidity,
                                           out vec4 aerosol_absorption,
                                           out vec4 aerosol_scattering,
                                           out vec4 molecular_absorption,
                                           out vec4 molecular_scattering,
                                           out vec4 extinction)
{
    h = max(h, 0.0); // In case height is negative
#if ENABLE_AEROSOLS == 0
    aerosol_absorption = vec4(0.0);
    aerosol_scattering = vec4(0.0);
#else
    float aerosol_density = get_aerosol_density(h, aerosol_type, aerosol_base_density, aerosol_height_scale, aerosol_background_divided_by_base_density);
    aerosol_absorption = aerosol_absorption_cross_section * aerosol_density * aerosol_turbidity;
    aerosol_scattering = aerosol_scattering_cross_section * aerosol_density * aerosol_turbidity;
#endif
    molecular_absorption = get_molecular_absorption_coefficient(h, ozone_mean_monthly_dobson);
    molecular_scattering = get_molecular_scattering_coefficient(h);
    extinction = aerosol_absorption + aerosol_scattering + molecular_absorption + molecular_scattering;
}

/*
 * Buffer A: Transmittance LUT
 *
 * In this buffer we precompute the transmittance to the top of the atmosphere.
 * We use the same technique as in "Precomputed Atmospheric Scattering"
 * by Eric Bruneton and Fabrice Neyret (2008).
 */

void MAIN()
{
    vec2 uv = INPUT_UV;
    float sun_cos_theta = uv.x * 2.0 - 1.0;
    vec3 sun_dir = vec3(-sqrt(1.0 - sun_cos_theta*sun_cos_theta), 0.0, sun_cos_theta);

    float distance_to_earth_center = mix(EARTH_RADIUS, ATMOSPHERE_RADIUS, uv.y);
    vec3 ray_origin = vec3(0.0, 0.0, distance_to_earth_center);

    float t_d = ray_sphere_intersection(ray_origin, sun_dir, ATMOSPHERE_RADIUS);
    float dt = t_d / float(TRANSMITTANCE_STEPS);

    vec4 result = vec4(0.0);

    for (int i = 0; i < TRANSMITTANCE_STEPS; ++i) {
        float t = (float(i) + 0.5) * dt;
        vec3 x_t = ray_origin + sun_dir * t;

        float altitude = length(x_t) - EARTH_RADIUS;

        vec4 aerosol_absorption, aerosol_scattering;
        vec4 molecular_absorption, molecular_scattering;
        vec4 extinction;
        get_atmosphere_collision_coefficients(
            altitude,
            aerosol_absorption_cross_section,
            aerosol_scattering_cross_section,
            aerosol_base_density,
            aerosol_height_scale,
            aerosol_background_divided_by_base_density,
            aerosol_type,
            ozone_mean_monthly_dobson,
            aerosol_turbidity,
            aerosol_absorption, aerosol_scattering,
            molecular_absorption, molecular_scattering,
            extinction);

        result += extinction * dt;
    }

    vec4 transmittance = exp(-result);
    FRAGCOLOR = transmittance;
}
