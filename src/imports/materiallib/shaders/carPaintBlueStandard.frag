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
#define texture_coordinate_uvw 0
#define texture_coordinate_world 1
#define texture_coordinate_object 2
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


struct texture_return
{
  vec3 tint;
  float mono;
};


// temporary declarations
texture_coordinate_info tmp2;
vec3 ftmp0;
float ftmp1;
 vec4 tmpShadowTerm;

layer_result layers[2];

#include "SSAOCustomMaterial.glsllib"
#include "sampleLight.glsllib"
#include "sampleProbe.glsllib"
#include "sampleArea.glsllib"
#include "cube.glsllib"
#include "random255.glsllib"
#include "perlinNoise.glsllib"
#include "perlinNoiseBumpTexture.glsllib"
#include "coordinateSource.glsllib"
#include "square.glsllib"
#include "calculateRoughness.glsllib"
#include "evalBakedShadowMap.glsllib"
#include "evalEnvironmentMap.glsllib"
#include "luminance.glsllib"
#include "microfacetBSDF.glsllib"
#include "physGlossyBSDF.glsllib"
#include "simpleGlossyBSDF.glsllib"
#include "weightedLayer.glsllib"
#include "miNoise.glsllib"
#include "flakeNoiseTexture.glsllib"
#include "directionalFactor.glsllib"
#include "diffuseReflectionBSDF.glsllib"
#include "fresnelLayer.glsllib"

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
  layers[0].layer += tmpShadowTerm * microfacetBSDF( layers[0].tanFrame, lightDir, viewDir, lightSpecular, materialIOR, 0.000000, 0.000000, scatter_reflect );

  layers[1].base += tmpShadowTerm * directionalFactor( normal, viewDir, normal_base_color, grazing_base_color, 5.000000, diffuseReflectionBSDF( normal, lightDir, viewDir, lightDiffuse, 0.000000 ) );
  layers[1].layer += tmpShadowTerm * microfacetBSDF( layers[1].tanFrame, lightDir, viewDir, lightSpecular, materialIOR, 0.200000, 0.200000, scatter_reflect );

#endif
}

void computeFrontAreaColor( in int lightIdx, in vec4 lightDiffuse, in vec4 lightSpecular )
{
#if QSSG_ENABLE_CG_LIGHTING
  layers[0].layer += tmpShadowTerm * lightSpecular * sampleAreaGlossy( layers[0].tanFrame, varWorldPos, lightIdx, viewDir, 0.000000, 0.000000 );

  layers[1].base += tmpShadowTerm * directionalFactor( normal, viewDir, normal_base_color, grazing_base_color, 5.000000, lightDiffuse * sampleAreaDiffuse( layers[1].tanFrame, varWorldPos, lightIdx ) );
  layers[1].layer += tmpShadowTerm * lightSpecular * sampleAreaGlossy( layers[1].tanFrame, varWorldPos, lightIdx, viewDir, 0.200000, 0.200000 );

#endif
}

void computeFrontLayerEnvironment( in vec3 normal, in vec3 viewDir, float aoFactor )
{
#if !QSSG_ENABLE_LIGHT_PROBE
  layers[0].layer += tmpShadowTerm * microfacetSampledBSDF( layers[0].tanFrame, viewDir, 0.000000, 0.000000, scatter_reflect );

  layers[1].base += tmpShadowTerm * directionalFactor( normal, viewDir, normal_base_color, grazing_base_color, 5.000000, diffuseReflectionBSDFEnvironment( normal, 0.000000 ) * aoFactor );
  layers[1].layer += tmpShadowTerm * microfacetSampledBSDF( layers[1].tanFrame, viewDir, 0.200000, 0.200000, scatter_reflect );

#else
  layers[0].layer += tmpShadowTerm * sampleGlossyAniso( layers[0].tanFrame, viewDir, 0.000000, 0.000000 );

  layers[1].base += tmpShadowTerm * directionalFactor( normal, viewDir, normal_base_color, grazing_base_color, 5.000000, sampleDiffuse( layers[1].tanFrame ) * aoFactor );
  layers[1].layer += tmpShadowTerm * sampleGlossyAniso( layers[1].tanFrame, viewDir, 0.200000, 0.200000 );

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
     tmp2 = coordinateSource(texture_coordinate_object, 0 );
     ftmp0 = perlinNoiseBumpTexture( tmp2, ( peel_amount/10.000000 ), ( peel_size*unit_conversion ), false, false, 0.000000, 1, false, vec3( 0.000000, 0.000000, 0.000000 ), 1.000000, 0.000000, 1.000000, normal );
     ftmp1 = flakeNoiseTexture(tmp2, 0.000000, ( unit_conversion*0.002000 ), 0.350000 ).mono;
     tmpShadowTerm = evalBakedShadowMap( texCoord0 );
}

vec4 computeLayerWeights( in float alpha )
{
  vec4 color;
  color = weightedLayer( ftmp1, vec4( vec3( 1, 0.117, 0.087 ), 1.0).rgb, layers[1].layer, layers[1].base * vec4( vec3( 1, 1, 1 ), 1.0), alpha );
  color = fresnelLayer( ftmp0, vec3( 1.5, 1.5, 1.5 ), 1.000000, vec4( vec3( 1, 1, 1 ), 1.0).rgb, layers[0].layer, color, color.a );
  return color;
}


void initializeLayerVariables(void)
{
  // clear layers
  layers[0].base = vec4(0.0, 0.0, 0.0, 1.0);
  layers[0].layer = vec4(0.0, 0.0, 0.0, 1.0);
  layers[0].tanFrame = orthoNormalize( mat3( tangent, cross(ftmp0, tangent), ftmp0 ) );
  layers[1].base = vec4(0.0, 0.0, 0.0, 1.0);
  layers[1].layer = vec4(0.0, 0.0, 0.0, 1.0);
  layers[1].tanFrame = orthoNormalize( mat3( tangent, cross(normal, tangent), normal ) );
}
