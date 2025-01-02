// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include "qssgrendergraphobject_p.h"

QT_BEGIN_NAMESPACE

QSSGRenderGraphObject::~QSSGRenderGraphObject() {}

const char *QSSGRenderGraphObject::asString(Type type)
{
#define RETURN_AS_STRING(T) case T: return #T;
    switch (type) {
        RETURN_AS_STRING(Type::Unknown)
        RETURN_AS_STRING(Type::Node)
        RETURN_AS_STRING(Type::Layer)
        RETURN_AS_STRING(Type::Joint)
        RETURN_AS_STRING(Type::Skeleton)
        RETURN_AS_STRING(Type::ImportScene)
        RETURN_AS_STRING(Type::ReflectionProbe)
        RETURN_AS_STRING(Type::DirectionalLight)
        RETURN_AS_STRING(Type::PointLight)
        RETURN_AS_STRING(Type::SpotLight)
        RETURN_AS_STRING(Type::OrthographicCamera)
        RETURN_AS_STRING(Type::PerspectiveCamera)
        RETURN_AS_STRING(Type::CustomFrustumCamera)
        RETURN_AS_STRING(Type::CustomCamera)
        RETURN_AS_STRING(Type::Model)
        RETURN_AS_STRING(Type::Item2D)
        RETURN_AS_STRING(Type::Particles)
        RETURN_AS_STRING(Type::SceneEnvironment)
        RETURN_AS_STRING(Type::Effect)
        RETURN_AS_STRING(Type::Geometry)
        RETURN_AS_STRING(Type::TextureData)
        RETURN_AS_STRING(Type::MorphTarget)
        RETURN_AS_STRING(Type::ModelInstance)
        RETURN_AS_STRING(Type::ModelBlendParticle)
        RETURN_AS_STRING(Type::ResourceLoader)
        RETURN_AS_STRING(Type::DefaultMaterial)
        RETURN_AS_STRING(Type::PrincipledMaterial)
        RETURN_AS_STRING(Type::CustomMaterial)
        RETURN_AS_STRING(Type::SpecularGlossyMaterial)
        RETURN_AS_STRING(Type::Skin)
        RETURN_AS_STRING(Type::Image2D)
        RETURN_AS_STRING(Type::ImageCube)
    }
#undef RETURN_AS_STRING
    return nullptr;
}

QDebug QSSGRenderGraphObject::debugPrintImpl(QDebug stream, Type type)
{
    stream.nospace() << "QSSGRenderGraphObject" << '{' << asString(type) << '}';
    return stream;
}

QT_END_NAMESPACE
