/****************************************************************************
**
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

#ifndef QSSGITEM2D_H
#define QSSGITEM2D_H

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
#include <QtQuick3D/private/qquick3dscenemanager_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQuick/QSGNode>
#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE

class QSGLayer;
class QQuick3DItem2D : public QQuick3DNode, public QQuickItemChangeListener
{
    Q_OBJECT
public:
    explicit QQuick3DItem2D(QQuickItem* item, QQuick3DNode *parent = nullptr);
    ~QQuick3DItem2D() override;

private Q_SLOTS:
    void sourceItemDestroyed(QObject *item);

private:
    void createLayerTexture();
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void markAllDirty() override;

    QQuickItem *m_sourceItem = nullptr;
    QSGLayer *m_layer = nullptr;
    QPointer<QQuick3DSceneManager> m_sceneManagerForLayer;
    bool m_initialized = false;
};

QT_END_NAMESPACE

#endif // QSSGITEM2D_H
