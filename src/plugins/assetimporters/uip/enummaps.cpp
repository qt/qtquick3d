/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
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

#include "enummaps.h"

QT_BEGIN_NAMESPACE

// When mapping NoXxxx to both "None" and "", "None" should come first to help
// strFromEnum give more readable results.

static EnumNameMap g_presentationRotationMap[] = {
    { UipPresentation::NoRotation, "None" },
    { UipPresentation::NoRotation, "" },
    { UipPresentation::NoRotation, "NoRotation" },
    { UipPresentation::Clockwise90, "90" },
    { UipPresentation::Clockwise180, "180" },
    { UipPresentation::Clockwise270, "270" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<UipPresentation::Rotation>::get()
{
    return g_presentationRotationMap;
}

static EnumNameMap g_nodeRotationOrderMap[] = {
    { Node::XYZ, "XYZ" },
    { Node::YZX, "YZX" },
    { Node::ZXY, "ZXY" },
    { Node::XZY, "XZY" },
    { Node::YXZ, "YXZ" },
    { Node::ZYX, "ZYX" },
    { Node::XYZr, "XYZr" },
    { Node::YZXr, "YZXr" },
    { Node::ZXYr, "ZXYr" },
    { Node::XZYr, "XZYr" },
    { Node::YXZr, "YXZr" },
    { Node::ZYXr, "ZYXr" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<Node::RotationOrder>::get()
{
    return g_nodeRotationOrderMap;
}

static EnumNameMap g_nodeOrientationMap[] = {
    { Node::LeftHanded, "Left Handed" },
    { Node::RightHanded, "Right Handed" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<Node::Orientation>::get()
{
    return g_nodeOrientationMap;
}

static EnumNameMap g_slidePlayModeMap[] = {
    { Slide::StopAtEnd, "Stop at end" },
    { Slide::Looping, "Looping" },
    { Slide::PingPong, "PingPong" },
    { Slide::Ping, "Ping" },
    { Slide::PlayThroughTo, "Play Through To..." },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<Slide::PlayMode>::get()
{
    return g_slidePlayModeMap;
}

static EnumNameMap g_slideInitialPlayStateMap[] = {
    { Slide::Play, "Play" },
    { Slide::Pause, "Pause" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<Slide::InitialPlayState>::get()
{
    return g_slideInitialPlayStateMap;
}

static EnumNameMap g_slidePlayThroughMap[] = {
    { Slide::Next, "Next" },
    { Slide::Previous, "Previous" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<Slide::PlayThrough>::get()
{
    return g_slidePlayThroughMap;
}

static EnumNameMap g_animationTrackAnimationType[] = {
    { AnimationTrack::NoAnimation, "None" },
    { AnimationTrack::NoAnimation, "" },
    { AnimationTrack::NoAnimation, "NoAnimation" },
    { AnimationTrack::Linear, "Linear" },
    { AnimationTrack::EaseInOut, "EaseInOut" },
    { AnimationTrack::Bezier, "Bezier" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<AnimationTrack::AnimationType>::get()
{
    return g_animationTrackAnimationType;
}

static EnumNameMap g_layerNodeProgressiveAA[] = {
    { LayerNode::NoPAA, "None" },
    { LayerNode::NoPAA, "" },
    { LayerNode::PAA2x, "2x" },
    { LayerNode::PAA4x, "4x" },
    { LayerNode::PAA8x, "8x" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<LayerNode::ProgressiveAA>::get()
{
    return g_layerNodeProgressiveAA;
}

static EnumNameMap g_layerNodeMultisampleAA[] = {
    { LayerNode::NoMSAA, "None" },
    { LayerNode::NoMSAA, "" },
    { LayerNode::MSAA2x, "2x" },
    { LayerNode::MSAA4x, "4x" },
    { LayerNode::SSAA, "SSAA" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<LayerNode::MultisampleAA>::get()
{
    return g_layerNodeMultisampleAA;
}

static EnumNameMap g_layerNodeLayerBackground[] = {
    { LayerNode::Transparent, "Transparent" },
    { LayerNode::SolidColor, "SolidColor" },
    { LayerNode::Unspecified, "Unspecified" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<LayerNode::LayerBackground>::get()
{
    return g_layerNodeLayerBackground;
}

static EnumNameMap g_layerNodeBlendType[] = {
    { LayerNode::Normal, "Normal" },
    { LayerNode::Screen, "Screen" },
    { LayerNode::Multiply, "Multiply" },
    { LayerNode::Add, "Add" },
    { LayerNode::Subtract, "Subtract" },
    { LayerNode::Overlay, "*Overlay" },
    { LayerNode::ColorBurn, "*ColorBurn" },
    { LayerNode::ColorDodge, "*ColorDodge" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<LayerNode::BlendType>::get()
{
    return g_layerNodeBlendType;
}

static EnumNameMap g_LayerNodehorzfields[] = {
    { LayerNode::LeftWidth, "Left/Width" },
    { LayerNode::LeftRight, "Left/Right" },
    { LayerNode::WidthRight, "Width/Right" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<LayerNode::HorizontalFields>::get()
{
    return g_LayerNodehorzfields;
}

static EnumNameMap g_LayerNodeUnits[] = {
    { LayerNode::Percent, "percent" },
    { LayerNode::Pixels, "pixels" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<LayerNode::Units>::get()
{
    return g_LayerNodeUnits;
}

static EnumNameMap g_LayerNodevertfields[] = {
    { LayerNode::TopHeight, "Top/Height" },
    { LayerNode::TopBottom, "Top/Bottom" },
    { LayerNode::HeightBottom, "Height/Bottom" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<LayerNode::VerticalFields>::get()
{
    return g_LayerNodevertfields;
}

static EnumNameMap g_Imagemappingmode[] = {
    { Image::UVMapping, "UV Mapping" },
    { Image::EnvironmentalMapping, "Environmental Mapping" },
    { Image::LightProbe, "Light Probe" },
    { Image::IBLOverride, "IBL Override" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<Image::MappingMode>::get()
{
    return g_Imagemappingmode;
}

static EnumNameMap g_Imagetilingmode[] = {
    { Image::Tiled, "Tiled" },
    { Image::Mirrored, "Mirrored" },
    { Image::NoTiling, "No Tiling" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<Image::TilingMode>::get()
{
    return g_Imagetilingmode;
}

static EnumNameMap g_ModelNodetessellation[] = {
    { ModelNode::None, "None" },
    { ModelNode::Linear, "Linear" },
    { ModelNode::Phong, "Phong" },
    { ModelNode::NPatch, "NPatch" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<ModelNode::Tessellation>::get()
{
    return g_ModelNodetessellation;
}

static EnumNameMap g_LightNodelighttype[] = {
    { LightNode::Directional, "Directional" },
    { LightNode::Point, "Point" },
    { LightNode::Area, "Area" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<LightNode::LightType>::get()
{
    return g_LightNodelighttype;
}

static EnumNameMap g_MaterialNodeshaderlighting[] = {
    { DefaultMaterial::PixelShaderLighting, "Pixel" },
    { DefaultMaterial::NoShaderLighting, "None" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<DefaultMaterial::ShaderLighting>::get()
{
    return g_MaterialNodeshaderlighting;
}

static EnumNameMap g_MaterialNodeblendmode[] = {
    { DefaultMaterial::Normal, "Normal" },
    { DefaultMaterial::Screen, "Screen" },
    { DefaultMaterial::Multiply, "Multiply" },
    { DefaultMaterial::Overlay, "*Overlay" },
    { DefaultMaterial::ColorBurn, "*ColorBurn" },
    { DefaultMaterial::ColorDodge, "*ColorDodge" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<DefaultMaterial::BlendMode>::get()
{
    return g_MaterialNodeblendmode;
}

static EnumNameMap g_MaterialNodespecularmodel[] = {
    { DefaultMaterial::DefaultSpecularModel, "Default" },
    { DefaultMaterial::KGGX, "KGGX" },
    { DefaultMaterial::KWard, "KWard" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<DefaultMaterial::SpecularModel>::get()
{
    return g_MaterialNodespecularmodel;
}

static EnumNameMap g_TextNodehorzalign[] = {
    { TextNode::Left, "Left" },
    { TextNode::Center, "Center" },
    { TextNode::Right, "Right" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<TextNode::HorizontalAlignment>::get()
{
    return g_TextNodehorzalign;
}

static EnumNameMap g_TextNodevertalign[] = {
    { TextNode::Top, "Top" },
    { TextNode::Middle, "Middle" },
    { TextNode::Bottom, "Bottom" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<TextNode::VerticalAlignment>::get()
{
    return g_TextNodevertalign;
}

static EnumNameMap g_TextNodewordwrap[] = {
    { TextNode::Clip, "Clip" },
    { TextNode::WrapWord, "WrapWord" },
    { TextNode::WrapAnywhere, "WrapAnywhere" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<TextNode::WordWrap>::get()
{
    return g_TextNodewordwrap;
}

static EnumNameMap g_TextNodeelide[] = {
    { TextNode::ElideNone, "ElideNone" },
    { TextNode::ElideLeft, "ElideLeft" },
    { TextNode::ElideMiddle, "ElideMiddle" },
    { TextNode::ElideRight, "ElideRight" },
    { 0, nullptr }
};

EnumNameMap *EnumParseMap<TextNode::Elide>::get()
{
    return g_TextNodeelide;
}

QT_END_NAMESPACE
