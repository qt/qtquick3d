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

#include <QtQuick3D/private/qquick3dnode_p.h>

class QQuick3DRuntimeLoader : public QQuick3DNode
{
    Q_OBJECT

    QML_NAMED_ELEMENT(RuntimeLoader)

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
public:
    QQuick3DRuntimeLoader();

    QUrl source() const;
    void setSource(const QUrl &newSource);
    void componentComplete() override;

    enum class Status { Empty, Success, Error };
    Q_ENUM(Status)
    Status status() const;
    QString errorString() const;

Q_SIGNALS:
    void sourceChanged();
    void statusChanged();
    void errorStringChanged();

private:
    void loadSource();
    QPointer<QQuick3DNode> m_imported;
    QUrl m_source;
    Status m_status = Status::Empty;
    QString m_errorString;
};

#endif // QQUICK3DRUNTIMELOADER_H
