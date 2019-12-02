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
#define gamma_default 0
#define gamma_linear 1
#define gamma_srgb 2
#define scatter_reflect 0
#define scatter_transmit 1
#define scatter_reflect_transmit 2

#define QSSG_ENABLE_UV0 1
#define QSSG_ENABLE_WORLD_POSITION 1
#define QSSG_ENABLE_TEXTAN 1
#define QSSG_ENABLE_BINORMAL 1

#include "vertexFragmentBase.glsllib"

// set shader output
out vec4 fragColor;

// add structure defines
struct texture_coordinate_info
{
  vec3 position;
  vec3 tangent_u;
  vec3 tangent_v;
};


struct layer_result
{
  vec4 base;
  vec4 layer;
  mat3 tanFrame;
};


struct texture_return
{
  vec3 tint;
  float mono;
};


// temporary declarations
texture_coordinate_info tmp1;
texture_coordinate_info tmp2;
float tmp3;
float ftmp0;
 vec4 tmpShadowTerm;

layer_result layer;

#include "SSAOCustomMaterial.glsllib"
#include "sampleLight.glsllib"
#include "sampleProbe.glsllib"
#include "sampleArea.glsllib"
#include "luminance.glsllib"
#include "monoChannel.glsllib"
#include "fileBumpTexture.glsllib"
#include "transformCoordinate.glsllib"
#include "rotationTranslationScale.glsllib"
#include "textureCoordinateInfo.glsllib"
#include "weightedLayer.glsllib"
#include "fileTexture.glsllib"
#include "square.glsllib"
#include "calculateRoughness.glsllib"
#include "evalBakedShadowMap.glsllib"
#include "evalEnvironmentMap.glsllib"
#include "microfacetBSDF.glsllib"
#include "physGlossyBSDF.glsllib"
#include "simpleGlossyBSDF.glsllib"

bool evalTwoSided()
{
  return( false );
}

vec3 computeFrontMaterialEmissive()
{
  return( vec3( 1.0, 1.0, 1.0) * vec3( vec3( ( intensity*( emission_color*( fileTexture(emissive_texture, vec3( 0, 0, 0 ), vec3( 1, 1, 1 ), mono_alpha, transformCoordinate( rotationTranslationScale( vec3( 0.000000, 0.000000, 0.000000 ), vec3( 0.000000, 0.000000, 0.000000 ), vec3( 1.000000, 1.000000, 1.000000 ) ), tmp1 ), vec2( 0.000000, 1.000000 ), vec2( 0.000000, 1.000000 ), wrap_repeat, wrap_repeat, gamma_default ).tint*fileTexture(emissive_mask_texture, vec3( 0, 0, 0 ), vec3( 1, 1, 1 ), mono_alpha, transformCoordinate( rotationTranslationScale( vec3( 0.000000, 0.000000, 0.000000 ), emissive_mask_offset, vec3( 1.000000, 1.000000, 1.000000 ) ), tmp1 ), vec2( 0.000000, 1.000000 ), vec2( 0.000000, 1.000000 ), wrap_repeat, wrap_repeat, gamma_default ).tint ) ) ) )  ) );
}

void computeFrontLayerColor( in vec3 normal, in vec3 lightDir, in vec3 viewDir, in vec3 lightDiffuse, in vec3 lightSpecular, in float materialIOR, float aoFactor )
{
#if QSSG_ENABLE_CG_LIGHTING
  layer.base += tmpShadowTerm * vec4( 0.0, 0.0, 0.0, 1.0 );
  layer.layer += tmpShadowTerm * microfacetBSDF( layer.tanFrame, lightDir, viewDir, lightSpecular, materialIOR, tmp3, tmp3, scatter_reflect );

#endif
}

void computeFrontAreaColor( in int lightIdx, in vec4 lightDiffuse, in vec4 lightSpecular )
{
#if QSSG_ENABLE_CG_LIGHTING
  layer.base += tmpShadowTerm * vec4( 0.0, 0.0, 0.0, 1.0 );
  layer.layer += tmpShadowTerm * lightSpecular * sampleAreaGlossy( layer.tanFrame, varWorldPos, lightIdx, viewDir, tmp3, tmp3 );

#endif
}

void computeFrontLayerEnvironment( in vec3 normal, in vec3 viewDir, float aoFactor )
{
#if !QSSG_ENABLE_LIGHT_PROBE
  layer.base += tmpShadowTerm * vec4( 0.0, 0.0, 0.0, 1.0 );
  layer.layer += tmpShadowTerm * microfacetSampledBSDF( layer.tanFrame, viewDir, tmp3, tmp3, scatter_reflect );

#else
  layer.base += tmpShadowTerm * vec4( 0.0, 0.0, 0.0, 1.0 );
  layer.layer += tmpShadowTerm * sampleGlossy( layer.tanFrame, viewDir, tmp3);

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
  return( false ? 1.0 : luminance( vec3( 1, 1, 1 ) ) );
}

float evalCutout()
{
  return( 1.000000 );
}

vec3 computeNormal()
{
  return( fileBumpTexture(bump_texture, bump_amount, mono_average, tmp2, vec2( 0.000000, 1.000000 ), vec2( 0.000000, 1.000000 ), wrap_repeat, wrap_repeat, normal ) );
}

void computeTemporaries()
{
     tmp1 = textureCoordinateInfo( texCoord0, tangent, binormal );
     tmp2 = transformCoordinate( rotationTranslationScale( vec3( 0.000000, 0.000000, 0.000000 ), vec3( 0.000000, 0.000000, 0.000000 ), tiling ), tmp1 );
     tmp3 = fileTexture(roughness_texture, vec3( roughness_map_offset ), vec3( roughness_map_scale ), mono_luminance, tmp2, vec2( 0.000000, 1.000000 ), vec2( 0.000000, 1.000000 ), wrap_repeat, wrap_repeat, gamma_default ).mono;
     ftmp0 = fileTexture(reflection_texture, vec3( reflection_map_offset ), vec3( reflection_map_scale ), mono_luminance, tmp2, vec2( 0.000000, 1.000000 ), vec2( 0.000000, 1.000000 ), wrap_repeat, wrap_repeat, gamma_default ).mono;
     tmpShadowTerm = evalBakedShadowMap( texCoord0 );
}

vec4 computeLayerWeights( in float alpha )
{
  vec4 color;
  color = weightedLayer( ftmp0, vec4( metal_color, 1.0).rgb, layer.layer, layer.base, alpha );
  return color;
}


void initializeLayerVariables(void)
{
  // clear layers
  layer.base = vec4(0.0, 0.0, 0.0, 1.0);
  layer.layer = vec4(0.0, 0.0, 0.0, 1.0);
  layer.tanFrame = orthoNormalize( mat3( tangent, cross(normal, tangent), normal ) );
}
