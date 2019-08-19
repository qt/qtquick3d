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
#define scatter_reflect 0
#define scatter_transmit 1
#define scatter_reflect_transmit 2
#define mono_alpha 0
#define mono_average 1
#define mono_luminance 2
#define mono_maximum 3
#define wrap_clamp 0
#define wrap_repeat 1
#define wrap_mirrored_repeat 2
#define gamma_default 0
#define gamma_linear 1
#define gamma_srgb 2

#define QSSG_ENABLE_UV0 1
#define QSSG_ENABLE_WORLD_POSITION 1
#define QSSG_ENABLE_TEXTAN 1
#define QSSG_ENABLE_BINORMAL 1

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


struct texture_coordinate_info
{
  vec3 position;
  vec3 tangent_u;
  vec3 tangent_v;
};


struct anisotropy_return
{
  float roughness_u;
  float roughness_v;
  vec3 tangent_u;
};


struct texture_return
{
  vec3 tint;
  float mono;
};


// temporary declarations
texture_coordinate_info tmp3;
texture_coordinate_info tmp4;
anisotropy_return tmp5;
vec3 tmp7;
float ftmp0;
float ftmp1;
vec3 ftmp2;
vec3 ftmp3;
vec3 ftmp4;
 vec4 tmpShadowTerm;

layer_result layers[3];

#include "SSAOCustomMaterial.glsllib"
#include "sampleLight.glsllib"
#include "sampleProbe.glsllib"
#include "sampleArea.glsllib"
#include "square.glsllib"
#include "calculateRoughness.glsllib"
#include "evalBakedShadowMap.glsllib"
#include "evalEnvironmentMap.glsllib"
#include "luminance.glsllib"
#include "microfacetBSDF.glsllib"
#include "physGlossyBSDF.glsllib"
#include "simpleGlossyBSDF.glsllib"
#include "monoChannel.glsllib"
#include "fileBumpTexture.glsllib"
#include "transformCoordinate.glsllib"
#include "rotationTranslationScale.glsllib"
#include "textureCoordinateInfo.glsllib"
#include "fileTexture.glsllib"
#include "anisotropyConversion.glsllib"
#include "weightedLayer.glsllib"
#include "diffuseReflectionBSDF.glsllib"
#include "fresnelLayer.glsllib"

bool evalTwoSided()
{
  return( false );
}

vec3 computeFrontMaterialEmissive()
{
  return( vec3( 1.0, 1.0, 1.0) * vec3( vec3( ( intensity*( emission_color*fileTexture(emissive_texture, vec3( 0, 0, 0 ), vec3( 1, 1, 1 ), mono_alpha, transformCoordinate( rotationTranslationScale( vec3( 0.000000, 0.000000, 0.000000 ), vec3( 0.000000, 0.000000, 0.000000 ), vec3( 1.000000, 1.000000, 1.000000 ) ), tmp3 ), vec2( 0.000000, 1.000000 ), vec2( 0.000000, 1.000000 ), wrap_repeat, wrap_repeat, gamma_default ).tint ) ) )  ) );
}

void computeFrontLayerColor( in vec3 normal, in vec3 lightDir, in vec3 viewDir, in vec3 lightDiffuse, in vec3 lightSpecular, in float materialIOR, float aoFactor )
{
#if QSSG_ENABLE_CG_LIGHTING
  layers[0].layer += tmpShadowTerm * microfacetBSDF( layers[0].tanFrame, lightDir, viewDir, lightSpecular, materialIOR, coat_roughness, coat_roughness, scatter_reflect );

  layers[1].layer += tmpShadowTerm * microfacetBSDF( layers[1].tanFrame, lightDir, viewDir, lightSpecular, materialIOR, ftmp0, ftmp1, scatter_reflect );

  layers[2].base += tmpShadowTerm * vec4( 0.0, 0.0, 0.0, 1.0 );
  layers[2].layer += tmpShadowTerm * diffuseReflectionBSDF( tmp7, lightDir, viewDir, lightDiffuse, 0.000000 );

#endif
}

void computeFrontAreaColor( in int lightIdx, in vec4 lightDiffuse, in vec4 lightSpecular )
{
#if QSSG_ENABLE_CG_LIGHTING
  layers[0].layer += tmpShadowTerm * lightSpecular * sampleAreaGlossy( layers[0].tanFrame, varWorldPos, lightIdx, viewDir, coat_roughness, coat_roughness );

  layers[1].layer += tmpShadowTerm * lightSpecular * sampleAreaGlossy( layers[1].tanFrame, varWorldPos, lightIdx, viewDir, ftmp0, ftmp1 );

  layers[2].base += tmpShadowTerm * vec4( 0.0, 0.0, 0.0, 1.0 );
  layers[2].layer += tmpShadowTerm * lightDiffuse * sampleAreaDiffuse( layers[2].tanFrame, varWorldPos, lightIdx );

#endif
}

void computeFrontLayerEnvironment( in vec3 normal, in vec3 viewDir, float aoFactor )
{
#if !QSSG_ENABLE_LIGHT_PROBE
  layers[0].layer += tmpShadowTerm * microfacetSampledBSDF( layers[0].tanFrame, viewDir, coat_roughness, coat_roughness, scatter_reflect );

  layers[1].layer += tmpShadowTerm * microfacetSampledBSDF( layers[1].tanFrame, viewDir, ftmp0, ftmp1, scatter_reflect );

  layers[2].base += tmpShadowTerm * vec4( 0.0, 0.0, 0.0, 1.0 );
  layers[2].layer += tmpShadowTerm * diffuseReflectionBSDFEnvironment( tmp7, 0.000000 ) * aoFactor;

#else
  layers[0].layer += tmpShadowTerm * sampleGlossyAniso( layers[0].tanFrame, viewDir, coat_roughness, coat_roughness );

  layers[1].layer += tmpShadowTerm * sampleGlossyAniso( layers[1].tanFrame, viewDir, ftmp0, ftmp1 );

  layers[2].base += tmpShadowTerm * vec4( 0.0, 0.0, 0.0, 1.0 );
  layers[2].layer += tmpShadowTerm * sampleDiffuse( layers[2].tanFrame ) * aoFactor;

#endif
}

vec3 computeBackMaterialEmissive()
{
  return( vec3(0, 0, 0) );
}

void computeBackLayerColor( in vec3 normal, in vec3 lightDir, in vec3 viewDir, in vec3 lightDiffuse, in vec3 lightSpecular, in float materialIOR, float aoFactor )
{
#if QSSG_ENABLE_CG_LIGHTING
  layers[0].base += vec4( 0.0, 0.0, 0.0, 1.0 );
  layers[0].layer += vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
}

void computeBackAreaColor( in int lightIdx, in vec4 lightDiffuse, in vec4 lightSpecular )
{
#if QSSG_ENABLE_CG_LIGHTING
  layers[0].base += vec4( 0.0, 0.0, 0.0, 1.0 );
  layers[0].layer += vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
}

void computeBackLayerEnvironment( in vec3 normal, in vec3 viewDir, float aoFactor )
{
#if !QSSG_ENABLE_LIGHT_PROBE
  layers[0].base += vec4( 0.0, 0.0, 0.0, 1.0 );
  layers[0].layer += vec4( 0.0, 0.0, 0.0, 1.0 );
#else
  layers[0].base += vec4( 0.0, 0.0, 0.0, 1.0 );
  layers[0].layer += vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
}

float computeIOR()
{
  return( false ? 1.0 : luminance( vec3( 1, 1, 1 ) ) );
}

float evalCutout()
{
  return( 1.000000 );
}

vec3 computeNormal()
{
  return( normal );
}

void computeTemporaries()
{
     tmp3 = textureCoordinateInfo( texCoord0, tangent, binormal );
     tmp4 = transformCoordinate( rotationTranslationScale( vec3( 0.000000, 0.000000, 0.000000 ), vec3( 0.000000, 0.000000, 0.000000 ), vec3( texture_tiling[0], texture_tiling[1], 1.000000 ) ), tmp3 );
     tmp5 = anisotropyConversion( base_roughness, anisotropy, fileTexture(anisotropy_rotation_texture, vec3( 0, 0, 0 ), vec3( 3.14, 3.14, 3.14 ), mono_luminance, tmp4, vec2( 0.000000, 1.000000 ), vec2( 0.000000, 1.000000 ), wrap_repeat, wrap_repeat, gamma_linear ).mono, tangent, false );
     tmp7 = fileBumpTexture(bump_texture, bump_amount, mono_average, tmp4, vec2( 0.000000, 1.000000 ), vec2( 0.000000, 1.000000 ), wrap_repeat, wrap_repeat, normal );
     ftmp0 = tmp5.roughness_u;
     ftmp1 = tmp5.roughness_v;
     ftmp2 = fileTexture(reflect_texture, vec3( 0, 0, 0 ), vec3( 1, 1, 1 ), mono_luminance, tmp4, vec2( 0.000000, 1.000000 ), vec2( 0.000000, 1.000000 ), wrap_repeat, wrap_repeat, gamma_linear ).tint;
     ftmp3 = tmp5.tangent_u;
     ftmp4 = fileTexture(diffuse_texture, vec3( 0, 0, 0 ), vec3( 1, 1, 1 ), mono_luminance, tmp4, vec2( 0.000000, 1.000000 ), vec2( 0.000000, 1.000000 ), wrap_repeat, wrap_repeat, gamma_srgb ).tint;
     tmpShadowTerm = evalBakedShadowMap( texCoord0 );
}

vec4 computeLayerWeights( in float alpha )
{
  vec4 color;
  color = weightedLayer( 1.000000, vec4( ftmp4, 1.0).rgb, layers[2].layer, layers[2].base, alpha );
  color = fresnelLayer( tmp7, vec3( base_ior ), base_weight, vec4( ftmp2, 1.0).rgb, layers[1].layer, color, color.a );
  color = fresnelLayer( normal, vec3( coat_ior ), coat_weight, vec4( vec3( 1, 1, 1 ), 1.0).rgb, layers[0].layer, color, color.a );
  return color;
}


void initializeLayerVariables(void)
{
  // clear layers
  layers[0].base = vec4(0.0, 0.0, 0.0, 1.0);
  layers[0].layer = vec4(0.0, 0.0, 0.0, 1.0);
  layers[0].tanFrame = orthoNormalize( mat3( tangent, cross(normal, tangent), normal ) );
  layers[1].base = vec4(0.0, 0.0, 0.0, 1.0);
  layers[1].layer = vec4(0.0, 0.0, 0.0, 1.0);
  layers[1].tanFrame = orthoNormalize( mat3( ftmp3, cross(tmp7, ftmp3), tmp7 ) );
  layers[2].base = vec4(0.0, 0.0, 0.0, 1.0);
  layers[2].layer = vec4(0.0, 0.0, 0.0, 1.0);
  layers[2].tanFrame = orthoNormalize( mat3( tangent, cross(tmp7, tangent), tmp7 ) );
}
