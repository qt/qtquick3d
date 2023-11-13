// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial


#include <QtQuick3DRuntimeRender/private/qssgrenderskeleton_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderSkeleton::QSSGRenderSkeleton()
    : QSSGRenderNode(QSSGRenderGraphObject::Type::Skeleton)
{
    boneTexData.setFormat(QSSGRenderTextureFormat::RGBA32F);
}

QSSGRenderSkeleton::~QSSGRenderSkeleton() = default;

QT_END_NAMESPACE
