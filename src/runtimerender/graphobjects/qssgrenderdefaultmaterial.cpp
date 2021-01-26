/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterial_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderDefaultMaterial::QSSGRenderDefaultMaterial(QSSGRenderGraphObject::Type type) : QSSGRenderGraphObject(type)
{
    Q_ASSERT(type == QSSGRenderGraphObject::Type::DefaultMaterial || type == QSSGRenderGraphObject::Type::PrincipledMaterial);
    if (type == QSSGRenderGraphObject::Type::PrincipledMaterial) {
        occlusionChannel = TextureChannelMapping::R;
        roughnessChannel = TextureChannelMapping::G;
        metalnessChannel = TextureChannelMapping::B;
    }
}

QT_END_NAMESPACE
