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

#ifndef QSSGSKELETON_H
#define QSSGSKELETON_H

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

#include <QtQuick3D/private/qquick3dnode_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3D_EXPORT QQuick3DSkeleton : public QQuick3DNode
{
    Q_OBJECT

    QML_NAMED_ELEMENT(Skeleton)

public:
    explicit QQuick3DSkeleton(QQuick3DNode *parent = nullptr);
    ~QQuick3DSkeleton() override;

public Q_SLOTS:

Q_SIGNALS:
    void skeletonNodeDirty();

private:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
};

QT_END_NAMESPACE

#endif // QSSGSKELETON_H
