/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#ifndef QSSGSKIN_H
#define QSSGSKIN_H

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

#include <QtQuick3D/qquick3dobject.h>
#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtCore/qlist.h>
#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3D_EXPORT QQuick3DSkin : public QQuick3DObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QQuick3DNode> joints READ joints)
    Q_PROPERTY(QList<QMatrix4x4> inverseBindPoses READ inverseBindPoses WRITE setInverseBindPoses NOTIFY inverseBindPosesChanged)

    QML_NAMED_ELEMENT(Skin)

public:
    explicit QQuick3DSkin(QQuick3DObject *parent = nullptr);
    ~QQuick3DSkin() override;

    QQmlListProperty<QQuick3DNode> joints();
    QList<QMatrix4x4> inverseBindPoses() const;

public Q_SLOTS:
    void setInverseBindPoses(const QList<QMatrix4x4> &poses);

Q_SIGNALS:
    void inverseBindPosesChanged();

private Q_SLOTS:
    void onJointChanged(QQuick3DNode *node);
    void onJointDestroyed(QObject *object);

private:
    void markDirty();
    void markAllDirty() override;

    static void qmlAppendJoint(QQmlListProperty<QQuick3DNode> *list, QQuick3DNode *joint);
    static QQuick3DNode *qmlJointAt(QQmlListProperty<QQuick3DNode> *list, qsizetype index);
    static qsizetype qmlJointsCount(QQmlListProperty<QQuick3DNode> *list);
    static void qmlClearJoints(QQmlListProperty<QQuick3DNode> *list);

    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;

    QVector<QQuick3DNode *> m_joints;
    QByteArray m_boneData;
    QList<QMatrix4x4> m_inverseBindPoses;
    bool m_dirty = false;
};

QT_END_NAMESPACE

#endif // QSSGSKIN_H
