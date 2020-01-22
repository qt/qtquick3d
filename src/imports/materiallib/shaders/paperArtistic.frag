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
#define wrap_clamp 0
#define wrap_repeat 1
#define wrap_mirrored_repeat 2
#define mono_alpha 0
#define mono_average 1
#define mono_luminance 2
#define mono_maximum 3
#define gamma_default 0
#define gamma_linear 1
#define gamma_srgb 2
#define color_layer_blend 0
#define color_layer_add 1
#define color_layer_multiply 2
#define color_layer_screen 3
#define color_layer_overlay 4
#define color_layer_brightness 5
#define color_layer_color 6

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


struct bsdf_component
{
  float weight;
  vec4 component;
};


struct color_layer
{
  vec3 layer_color;
  float weight;
  int mode;
};


struct texture_return
{
  vec3 tint;
  float mono;
};


struct layer_result
{
  vec4 base;
  vec4 layer;
  mat3 tanFrame;
};


// temporary declarations
texture_coordinate_info tmp1;
vec3 tmp2;
vec3 ftmp0;
 vec4 tmpShadowTerm;

layer_result layer;

#include "SSAOCustomMaterial.glsllib"
#include "sampleLight.glsllib"
#include "sampleProbe.glsllib"
#include "sampleArea.glsllib"
#include "tangentSpaceNormalTexture.glsllib"
#include "transformCoordinate.glsllib"
#include "rotationTranslationScale.glsllib"
#include "textureCoordinateInfo.glsllib"
#include "normalizedMix.glsllib"
#include "evalBakedShadowMap.glsllib"
#include "diffuseTransmissionBSDF.glsllib"
#include "luminance.glsllib"
#include "monoChannel.glsllib"
#include "fileTexture.glsllib"
#include "blendColorLayers.glsllib"
#include "diffuseReflectionBSDF.glsllib"

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
  layer.base += tmpShadowTerm * normalizedMix( bsdf_component[2]( bsdf_component(transmission_weight, diffuseTransmissionBSDF( -normal, lightDir, viewDir, lightDiffuse, vec4( ftmp0, 1.0), uTranslucentFalloff, uDiffuseLightWrap )) ,bsdf_component(reflection_weight, diffuseReflectionBSDF( normal, lightDir, lightDiffuse)) ) );

#endif
}

void computeFrontAreaColor( in int lightIdx, in vec4 lightDiffuse, in vec4 lightSpecular )
{
#if QSSG_ENABLE_CG_LIGHTING
  layer.base += tmpShadowTerm * normalizedMix( bsdf_component[2]( bsdf_component(transmission_weight, lightDiffuse * sampleAreaDiffuseTransmissive( layer.tanFrame, varWorldPos, lightIdx, vec4( ftmp0, 1.0), uTranslucentFalloff, uDiffuseLightWrap )) ,bsdf_component(reflection_weight, lightDiffuse * sampleAreaDiffuse( layer.tanFrame, varWorldPos, lightIdx )) ) );

#endif
}

void computeFrontLayerEnvironment( in vec3 normal, in vec3 viewDir, float aoFactor )
{
#if !QSSG_ENABLE_LIGHT_PROBE
  layer.base += tmpShadowTerm * diffuseReflectionBSDFEnvironment( normal, 0.000000 ) * aoFactor;

#else
  layer.base += tmpShadowTerm * sampleDiffuse( layer.tanFrame ) * aoFactor;

#endif
}

void computeFrontLayerRnmColor( in vec3 normal, in vec3 rnmX, in vec3 rnmY, in vec3 rnmZ )
{
#if QSSG_ENABLE_RNM
  layer.base += tmpShadowTerm * diffuseRNM( normal, rnmX, rnmY, rnmZ );

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

void computeBackLayerRnmColor( in vec3 normal, in vec3 rnmX, in vec3 rnmY, in vec3 rnmZ )
{
#if QSSG_ENABLE_RNM
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
  return( tangentSpaceNormalTexture( bump_texture, bump_amount, false, false, tmp1, vec2( 0.000000, 1.000000 ), vec2( 0.000000, 1.000000 ), wrap_repeat, wrap_repeat ) );
}

void computeTemporaries()
{
     tmp1 = transformCoordinate( rotationTranslationScale( vec3( 0.000000, 0.000000, 0.000000 ), vec3( 0.000000, 0.000000, 0.000000 ), vec3( texture_tiling[0], texture_tiling[1], 1.000000 ) ), textureCoordinateInfo( texCoord0, tangent, binormal ) );
     tmp2 = fileTexture(diffuse_texture, vec3( 0, 0, 0 ), vec3( 1, 1, 1 ), mono_luminance, tmp1, vec2( 0.000000, 1.000000 ), vec2( 0.000000, 1.000000 ), wrap_repeat, wrap_repeat, gamma_srgb ).tint;
     ftmp0 = blendColorLayers( color_layer[1]( color_layer(blendColorLayers( color_layer[1]( color_layer(fileTexture(transmission_texture, vec3( 0, 0, 0 ), vec3( 1, 1, 1 ), mono_luminance, tmp1, vec2( 0.000000, 1.000000 ), vec2( 0.000000, 1.000000 ), wrap_repeat, wrap_repeat, gamma_linear ).tint, 1.000000, color_layer_blend ) ), tmp2, mono_average ).tint, 1.000000, color_layer_multiply ) ), vec3( 1, 1, 1 ), mono_average ).tint;
     tmpShadowTerm = evalBakedShadowMap( texCoord0 );
}

vec4 computeLayerWeights( in float alpha )
{
  vec4 color;
  color = layer.base * vec4( tmp2, 1.0);
  return color;
}


void initializeLayerVariables(void)
{
  // clear layers
  layer.base = vec4(0.0, 0.0, 0.0, 1.0);
  layer.layer = vec4(0.0, 0.0, 0.0, 1.0);
  layer.tanFrame = orthoNormalize( tangentFrame( normal, varWorldPos ) );
}

vec4 computeOpacity(in vec4 color)
{
  vec4 rgba = color;
  rgba.a = uOpacity * 0.01;
  return rgba;
}
