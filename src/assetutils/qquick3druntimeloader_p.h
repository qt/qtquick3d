/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QQUICK3DRUNTIMELOADER_H
#define QQUICK3DRUNTIMELOADER_H

#include <QtQuick3D/private/qquick3dmodel_p.h>
#include <QtQuick3D/private/qquick3dinstancing_p.h>

#include "qtquick3dassetutilsglobal_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICK3DASSETUTILS_EXPORT QQuick3DRuntimeLoader : public QQuick3DNode
{
    Q_OBJECT

    QML_NAMED_ELEMENT(RuntimeLoader)
    QML_ADDED_IN_VERSION(6, 2)

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(QQuick3DBounds3 bounds READ bounds NOTIFY boundsChanged)
    Q_PROPERTY(QQuick3DInstancing *instancing READ instancing WRITE setInstancing NOTIFY instancingChanged)

public:
    explicit QQuick3DRuntimeLoader(QQuick3DNode *parent = nullptr);

    QUrl source() const;
    void setSource(const QUrl &newSource);
    void componentComplete() override;

    enum class Status { Empty, Success, Error };
    Q_ENUM(Status)
    Status status() const;
    QString errorString() const;
    const QQuick3DBounds3 &bounds() const;

    QQuick3DInstancing *instancing() const;
    void setInstancing(QQuick3DInstancing *newInstancing);

Q_SIGNALS:
    void sourceChanged();
    void statusChanged();
    void errorStringChanged();
    void boundsChanged();
    void instancingChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;

private:
    void calculateBounds();
    void loadSource();
    void updateModels();

    QPointer<QQuick3DNode> m_root;
    QPointer<QQuick3DNode> m_imported;
    QString m_assetId; // Used to release runtime assets in the buffer manager.
    QUrl m_source;
    Status m_status = Status::Empty;
    QString m_errorString;
    bool m_boundsDirty = false;
    QQuick3DBounds3 m_bounds;
    QQuick3DInstancing *m_instancing = nullptr;
    bool m_instancingChanged = false;
};

QT_END_NAMESPACE

#endif // QQUICK3DRUNTIMELOADER_H
