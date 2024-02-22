/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
#include <QtCore/QVector>

QT_BEGIN_NAMESPACE

class QSGLayer;
class QSGRenderer;
class QSGRootNode;
class QQuickRootItem;
class Q_AUTOTEST_EXPORT QQuick3DItem2D : public QQuick3DNode, public QQuickItemChangeListener
{
    Q_OBJECT
public:
    explicit QQuick3DItem2D(QQuickItem* item, QQuick3DNode *parent = nullptr);
    ~QQuick3DItem2D() override;

    void addChildItem(QQuickItem *item);
    void removeChildItem(QQuickItem *item);
    QQuickItem *contentItem() const;
    void itemDestroyed(QQuickItem *item) override;

private Q_SLOTS:
    void invalidated();
    void updatePicking();
    void derefWindow(QObject *win);

Q_SIGNALS:
    void allChildrenRemoved();

protected:
    void preSync() override;
    void itemChange(QQuick3DObject::ItemChange change, const QQuick3DObject::ItemChangeData &value) override;
private:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void markAllDirty() override;

    QVector<QQuickItem *> m_sourceItems;
    QSGRenderer *m_renderer = nullptr;
    QSGRootNode *m_rootNode = nullptr;
    QQuickWindow *m_window = nullptr;
    QQuickItem *m_contentItem = nullptr;
    bool m_sceneManagerValid = false;
    bool m_pickingDirty = true;
    bool m_updatingRendererNode = false;
    QPointer<QQuick3DSceneManager> m_sceneManagerForLayer;
};

QT_END_NAMESPACE

#endif // QSSGITEM2D_H
