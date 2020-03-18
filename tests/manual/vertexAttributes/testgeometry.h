/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef TESTGEOMETRY_H
#define TESTGEOMETRY_H

#include <QtQuick3D/qquick3d.h>
#include <QtQuick3D/qquick3dgeometry.h>

class TestGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    Q_PROPERTY(bool position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(bool normal READ normal WRITE setNormal NOTIFY normalChanged)
    Q_PROPERTY(bool texcoord0 READ texcoord0 WRITE setTexcoord0 NOTIFY texcoord0Changed)
    Q_PROPERTY(bool texcoord1 READ texcoord1 WRITE setTexcoord1 NOTIFY texcoord1Changed)
    Q_PROPERTY(bool tangent READ tangent WRITE setTangent NOTIFY tangentChanged)
    Q_PROPERTY(bool binormal READ binormal WRITE setBinormal NOTIFY binormalChanged)
    Q_PROPERTY(bool color READ color WRITE setColor NOTIFY colorChanged)

public:
    TestGeometry(QQuick3DObject *parent = nullptr);

    bool position() const;
    bool normal() const;
    bool texcoord0() const;
    bool texcoord1() const;
    bool tangent() const;
    bool binormal() const;
    bool color() const;

public Q_SLOTS:
    void setPosition(bool enable);
    void setNormal(bool enable);
    void setTexcoord0(bool enable);
    void setTexcoord1(bool enable);
    void setTangent(bool enable);
    void setBinormal(bool enable);
    void setColor(bool enable);

Q_SIGNALS:
    void positionChanged();
    void normalChanged();
    void texcoord0Changed();
    void texcoord1Changed();
    void tangentChanged();
    void binormalChanged();
    void colorChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;

private:
    void updateId();

    bool m_position = false;
    bool m_normal = false;
    bool m_texcoord0 = false;
    bool m_texcoord1 = false;
    bool m_tangent = false;
    bool m_binormal = false;
    bool m_color = false;
    bool m_dirty = true;

    QByteArray m_id;
    QByteArray m_vertexBuffer;
    QByteArray m_indexBuffer;
};

#endif // TESTGEOMETRY_H
