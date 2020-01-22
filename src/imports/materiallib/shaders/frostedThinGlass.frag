/****************************************************************************
**
** Copyright (C) 2014 NVIDIA Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

// add enum defines
#define mono_alpha 0
#define mono_average 1
#define mono_luminance 2
#define mono_maximum 3
#define wrap_clamp 0
#define wrap_repeat 1
#define wrap_mirrored_repeat 2
#define texture_coordinate_uvw 0
#define texture_coordinate_world 1
#define texture_coordinate_object 2
#define scatter_reflect 0
#define scatter_transmit 1
#define scatter_reflect_transmit 2

#define QSSG_ENABLE_UV0 1
#define QSSG_ENABLE_WORLD_POSITION 1
#define QSSG_ENABLE_TEXTAN 1
#define QSSG_ENABLE_BINORMAL 0

#include "vertexFragmentBase.glsllib"

// set shader output
out vec4 fragColor;

// add structure defines
struct layer_result
{
  vec4 base;
  vec4 layer;
  mat3 tanFrame;
};

// add structure defines
struct texture_coordinate_info
{
  vec3 position;
  vec3 tangent_u;
  vec3 tangent_v;
};

struct texture_return
{
  vec3 tint;
  float mono;
};

// temporary declarations
texture_coordinate_info tmp0;
texture_coordinate_info tmp1;
vec3 ftmp0;
vec3 ftmp1;
vec3 ftmp2;
vec4 tmpShadowTerm;

layer_result layer;

#include "SSAOCustomMaterial.glsllib"
#include "sampleLight.glsllib"
#include "sampleProbe.glsllib"
#include "sampleArea.glsllib"
#include "square.glsllib"
#include "cube.glsllib"
#include "random255.glsllib"
#include "perlinNoise.glsllib"
#include "perlinNoiseBumpTexture.glsllib"
#include "luminance.glsllib"
#include "monoChannel.glsllib"
#include "fileBumpTexture.glsllib"
#include "transformCoordinate.glsllib"
#include "rotationTranslationScale.glsllib"
#include "coordinateSource.glsllib"
#include "calculateRoughness.glsllib"
#include "evalBakedShadowMap.glsllib"
#include "evalEnvironmentMap.glsllib"
#include "microfacetBSDF.glsllib"
#include "physGlossyBSDF.glsllib"
#include "simpleGlossyBSDF.glsllib"
#include "abbeNumberIOR.glsllib"
#include "average.glsllib"
#include "perlinNoiseTexture.glsllib"
#include "fresnelLayer.glsllib"
#include "refraction.glsllib"

uniform sampler2D refractiveTexture;

bool evalTwoSided()
{
  return( false );
}

vec3 computeFrontMaterialEmissive()
{
  return( vec3( 0, 0, 0 ) );
}

void computeFrontLayerColor( in vec3 normal, in vec3 lightDir, in vec3 viewDir, in vec3 lightDiffuse, in vec3 lightSpecular, in float materialIOR, float aoFactor )
{
#if QSSG_ENABLE_CG_LIGHTING
  layer.base += tmpShadowTerm * microfacetBSDF( layer.tanFrame, lightDir, viewDir, lightSpecular, materialIOR, roughness, roughness, scatter_reflect_transmit );

#endif
}

void computeFrontAreaColor( in int lightIdx, in vec4 lightDiffuse, in vec4 lightSpecular )
{
#if QSSG_ENABLE_CG_LIGHTING
  layer.base += tmpShadowTerm * lightSpecular * sampleAreaGlossy( layer.tanFrame, varWorldPos, lightIdx, viewDir, roughness, roughness );

#endif
}

void computeFrontLayerEnvironment( in vec3 normal, in vec3 viewDir, float aoFactor )
{
#if !QSSG_ENABLE_LIGHT_PROBE
  layer.base += tmpShadowTerm * microfacetSampledBSDF( layer.tanFrame, viewDir, roughness, roughness, scatter_reflect_transmit );

#else
  layer.base += tmpShadowTerm * sampleGlossy( layer.tanFrame, viewDir, roughness);

#endif
}

vec3 computeBackMaterialEmissive()
{
  return( vec3(0, 0, 0) );
}

void computeBackLayerColor( in vec3 normal, in vec3 lightDir, in vec3 viewDir, in vec3 lightDiffuse, in vec3 lightSpecular, in float materialIOR, float aoFactor )
{
#if QSSG_ENABLE_CG_LIGHTING
  layer.base += vec4( 0.0, 0.0, 0.0, 1.0 );
  layer.layer += vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
}

void computeBackAreaColor( in int lightIdx, in vec4 lightDiffuse, in vec4 lightSpecular )
{
#if QSSG_ENABLE_CG_LIGHTING
  layer.base += vec4( 0.0, 0.0, 0.0, 1.0 );
  layer.layer += vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
}

void computeBackLayerEnvironment( in vec3 normal, in vec3 viewDir, float aoFactor )
{
#if !QSSG_ENABLE_LIGHT_PROBE
  layer.base += vec4( 0.0, 0.0, 0.0, 1.0 );
  layer.layer += vec4( 0.0, 0.0, 0.0, 1.0 );
#else
  layer.base += vec4( 0.0, 0.0, 0.0, 1.0 );
  layer.layer += vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
}

float computeIOR()
{
  return( false ? 1.0f : luminance( vec3( abbeNumberIOR(glass_ior, 0.000000 ) ) ) );
}

float evalCutout()
{
  return( 1.000000 );
}

vec3 computeNormal()
{
  if ( glass_bfactor > 0.0 )
  {
    ftmp2 = fileBumpTexture(glass_bump, glass_bfactor, mono_average, tmp0, vec2( 0.000000, 1.000000 ), vec2( 0.000000, 1.000000 ), wrap_repeat, wrap_repeat, normal );
    if (!glass_binside) { normal = ftmp2; }
  }

  return( perlinNoiseBumpTexture( tmp1, bumpScale, 1.000000, false, false, 0.000000, bumpBands, false, vec3( 0.000000, 0.000000, 0.000000 ), 0.5, 0.0, 1.000000, normal ) );
}

void computeTemporaries()
{
    //tmp0 = transformCoordinate( rotationTranslationScale( vec3( 0.000000, 0.000000, 0.000000 ), vec3( 0.000000, 0.000000, 0.000000 ), refractCoords ), coordinateSource(texture_coordinate_world, 0 ) );
    //ftmp1 = perlinNoiseBumpTexture( tmp0, refractScale, 1.000000, false, false, 0.000000, 1, false, vec3( 0.000000, 0.000000, 0.000000 ), 1.0, 0.5, 1.000000, viewDir );
    tmp0 = transformCoordinate( rotationTranslationScale( vec3( 0.000000, 0.000000, 0.000000 ), vec3( 0.000000, 0.000000, 0.000000 ), bumpCoords ), textureCoordinateInfo( texCoord0, tangent, binormal ) );
    tmp1 = transformCoordinate( rotationTranslationScale( vec3( 0.000000, 0.000000, 0.000000 ), vec3( 0.000000, 0.000000, 0.000000 ), bumpCoords ), coordinateSource(texture_coordinate_world, 0 ) );
    ftmp1 = viewDir;
    ftmp0 = vec3( reflectivity_amount );
    tmpShadowTerm = evalBakedShadowMap( texCoord0 );
}

vec4 computeLayerWeights( in float alpha )
{
  vec4 color;
  color = layer.base * vec4( ftmp0, 1.0);
  return color;
}


void initializeLayerVariables(void)
{
  // clear layers
  layer.base = vec4(0.0, 0.0, 0.0, 1.0);
  layer.layer = vec4(0.0, 0.0, 0.0, 1.0);
  layer.tanFrame = orthoNormalize( tangentFrame( normal, varWorldPos ) );
}

vec3 getRefractUV( in vec2 baseUV, in vec3 normal, in float materialIOR, in float refractDepth )
{
    // Real honest-to-goodness refraction!
    vec3 refractedDir = refract( -viewDir, normal, 1.0 / materialIOR );
    float thickness = refractDepth / clamp( dot(viewDir, normal), 0.0001, 1.0 );

    // This will do an "AA" version of that loss due to critical angle and TIR
        // fakes the same effect than using the glsl refract.
    float weight = smoothstep( 0.0, 1.0, abs(dot(viewDir, normal)) * 100.0 );

    // Trace out the refracted ray and the straight view ray
    refractedDir *= thickness;
    vec3 rawDir = -viewDir * thickness;

    vec3 displace = refractedDir - rawDir;
    vec3 newUV = vec3(baseUV + displace.xy, weight);
    return newUV;
}

vec4 doFakeInnerLight( in vec3 normal, in vec3 absorb_color )
{
    vec3 lightColor = intLightCol * intLightBrt;

    float cosRot = cos(intLightRot * 0.01745329251);
    float sinRot = sin(intLightRot * 0.01745329251);
    vec2 uvDir = vec2(sinRot, cosRot);

    vec2 dvec = texCoord0.xy - intLightPos;
    float dist = dot( dvec, uvDir );
    float fallRate = log2( max( abs(intLightFall), 1.01 ) );
    vec3 fallCol = exp2( -abs(dist) * fallRate / absorb_color );

    vec3 projDir = (tangent * uvDir.x + binormal * uvDir.y) * dist * intLightFall - surfNormal * refract_depth;
    projDir = normalize(projDir);

    vec4 retVal = vec4(lightColor * fallCol, 1.0);
    retVal *= abs(dot( projDir, -ftmp2 ));
    retVal.a = pow( retVal.a, uFresnelPower );
    retVal.a *= clamp( intLightBrt * exp2(-dist * fallRate), 0.0, 1.0 );

    return retVal;
}

vec4 computeGlass(in vec3 normal, in float materialIOR, in float alpha, in vec4 color)
{
  vec4 rgba = color;
  float ratio = simpleFresnel( normal, materialIOR, uFresnelPower );
  vec3 absorb_color = ( log( glass_color ) * -1.000000 );
  // prevent log(0) -> inf number issue
  if ( isinf(absorb_color.r) ) absorb_color.r = 1.0;
  if ( isinf(absorb_color.g) ) absorb_color.g = 1.0;
  if ( isinf(absorb_color.b) ) absorb_color.b = 1.0;
  rgba.rgb *= (vec3(1.0) - absorb_color);

  vec2 texSize = vec2( textureSize( refractiveTexture, 0 ) );
  vec3 newUV = vec3((gl_FragCoord.xy * 0.5) / texSize, 0.0);
  vec4 value = texture( refractiveTexture, newUV.xy );

  newUV = getRefractUV( newUV.xy, normal, materialIOR, 0.01 * refract_depth );
  vec4 refractValue = texture( refractiveTexture, newUV.xy );

  vec3 refractColor = refractValue.a * refractValue.rgb + (1.0 - refractValue.a) * value.rgb;
  refractColor = refractColor * (vec3(1.0) - absorb_color);
  vec4 internalColor = doFakeInnerLight( normal, glass_color );
  refractColor += internalColor.rgb * internalColor.a;

  rgba = vec4(mix(refractColor, rgba.rgb, ratio), 1.0);
  return rgba;
}
