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


// temporary declarations
vec3 ftmp0;
 vec4 tmpShadowTerm;

layer_result layer;

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
#include "abbeNumberIOR.glsllib"
#include "fresnelLayer.glsllib"
#include "refraction.glsllib"

bool evalTwoSided()
{
  return( true );
}

vec3 computeFrontMaterialEmissive()
{
  return( vec3( 0, 0, 0 ) );
}

void computeFrontLayerColor( in vec3 normal, in vec3 lightDir, in vec3 viewDir, in vec3 lightDiffuse, in vec3 lightSpecular, in float materialIOR, float aoFactor )
{
#if QSSG_ENABLE_CG_LIGHTING
  layer.base += tmpShadowTerm * microfacetBSDF( layer.tanFrame, lightDir, viewDir, lightSpecular, materialIOR, 0.000000, 0.000000, scatter_reflect_transmit );
#endif
}

void computeFrontAreaColor( in int lightIdx, in vec4 lightDiffuse, in vec4 lightSpecular )
{
#if QSSG_ENABLE_CG_LIGHTING
  layer.base += tmpShadowTerm * lightSpecular * sampleAreaGlossy( layer.tanFrame, varWorldPos, lightIdx, viewDir, 0.000000, 0.000000 );
#endif
}

void computeFrontLayerEnvironment( in vec3 normal, in vec3 viewDir, float aoFactor )
{
#if !QSSG_ENABLE_LIGHT_PROBE
  layer.base += tmpShadowTerm * microfacetSampledBSDF( layer.tanFrame, viewDir, 0.000000, 0.000000, scatter_reflect_transmit );
#else
  layer.base += tmpShadowTerm * sampleGlossy( layer.tanFrame, viewDir, 0.000000);
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
  return( true ? 1.0f : luminance( vec3( abbeNumberIOR(glass_ior, 0.000000 ) ) ) );
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

vec4 computeGlass(in vec3 normal, in float materialIOR, in float alpha, in vec4 color)
{
  vec4 rgba = color;
  float ratio = simpleFresnel( normal, materialIOR, uFresnelPower );
  vec3 absorb_color = ( log( glass_color )/-1.000000 );
  // prevent log(0) -> inf number issue
  if ( isinf(absorb_color.r) ) absorb_color.r = 1.0;
  if ( isinf(absorb_color.g) ) absorb_color.g = 1.0;
  if ( isinf(absorb_color.b) ) absorb_color.b = 1.0;
  rgba.rgb = mix(vec3(1.0) - absorb_color, rgba.rgb * (vec3(1.0) - absorb_color), ratio);
  rgba.a = mix(uMinOpacity, alpha, ratio);
  return rgba;
}
