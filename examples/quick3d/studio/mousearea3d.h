/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOUSEAREA3D
#define MOUSEAREA3D

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

#include <QVector3D>
#include <QtCore/qpointer.h>
#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3D/private/qtquick3dglobal_p.h>


class MouseArea3D : public QQuick3DNode
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DViewport *view3D READ view3D WRITE setView3D NOTIFY view3DChanged)
    Q_PROPERTY(bool grabsMouse READ grabsMouse WRITE setGrabsMouse NOTIFY grabsMouseChanged)
    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged)
    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(qreal height READ height WRITE setHeight NOTIFY heightChanged)
    Q_PROPERTY(bool hovering READ hovering NOTIFY hoveringChanged)
    Q_PROPERTY(bool dragging READ dragging NOTIFY draggingChanged)

    Q_INTERFACES(QQmlParserStatus)

public:
    MouseArea3D(QQuick3DNode *parent = nullptr);

    QQuick3DViewport *view3D() const;

    qreal x() const;
    qreal y() const;
    qreal width() const;
    qreal height() const;

    bool hovering() const;
    bool dragging() const;
    bool grabsMouse() const;

public slots:
    void setView3D(QQuick3DViewport *view3D);
    void setGrabsMouse(bool grabsMouse);

    void setX(qreal x);
    void setY(qreal y);
    void setWidth(qreal width);
    void setHeight(qreal height);

    Q_INVOKABLE QVector3D rayIntersectsPlane(const QVector3D &rayPos0, const QVector3D &rayPos1, const QVector3D &planePos, const QVector3D &planeNormal) const;

signals:
    void view3DChanged();

    void xChanged(qreal x);
    void yChanged(qreal y);
    void widthChanged(qreal width);
    void heightChanged(qreal height);

    void hoveringChanged();
    void draggingChanged();
    void pressed(const QVector3D &pointerPosition);
    void released(const QVector3D &pointerPosition);
    void dragged(const QVector3D &pointerPosition);
    void grabsMouseChanged(bool grabsMouse);

protected:
    void classBegin() override {}
    void componentComplete() override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Q_DISABLE_COPY(MouseArea3D)
    QQuick3DViewport *m_view3D = nullptr;

    qreal m_x;
    qreal m_y;
    qreal m_width;
    qreal m_height;

    bool m_hovering = false;
    bool m_dragging = false;

    QVector3D getMousePosInPlane(const QPointF mousePosInView) const;

private:
    static MouseArea3D *s_mouseGrab;
    bool m_grabsMouse;
};


QML_DECLARE_TYPE(MouseArea3D)

#endif // MOUSEPOINT3D
