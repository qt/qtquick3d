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

#include "qssgplane_p.h"

QT_BEGIN_NAMESPACE

namespace {
float magnitude(const QVector3D &vector)
{
    return std::sqrt(vector.x() * vector.x() + vector.y() * vector.y() + vector.z() * vector.z());
}
}

void QSSGPlane::normalize()
{
    float denom = 1.0f / magnitude(n);
    n *= denom;
    d *= denom;
}

QT_END_NAMESPACE
