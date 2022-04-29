/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QSSG_RENDER_MORPH_TARGET_H
#define QSSG_RENDER_MORPH_TARGET_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererutil_p.h>

#include <QtCore/QVector>

QT_BEGIN_NAMESPACE

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderMorphTarget : public QSSGRenderGraphObject
{
    enum class InputAttribute : quint8
    {
        Position   = 0x01,
        Normal     = 0x02,
        Tangent    = 0x04,
        Binormal   = 0x08
    };
    Q_DECLARE_FLAGS(InputAttributes, InputAttribute);

    float weight = 0.0f;
    InputAttributes attributes;

    QSSGRenderMorphTarget();
    ~QSSGRenderMorphTarget();
};
QT_END_NAMESPACE

#endif
