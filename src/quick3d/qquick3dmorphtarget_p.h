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

#ifndef QSSGMORPHTARGET_H
#define QSSGMORPHTARGET_H

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

#include <QtQuick3D/private/qquick3dobject_p.h>

#include <QtQml/QQmlListProperty>

#include <QtCore/QVector>
#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE
class QQuick3DModel;

class Q_QUICK3D_EXPORT QQuick3DMorphTarget : public QQuick3DObject
{
    Q_OBJECT
    Q_PROPERTY(float weight READ weight WRITE setWeight NOTIFY weightChanged)
    Q_PROPERTY(MorphTargetAttributes attributes READ attributes WRITE setAttributes NOTIFY attributesChanged)
    QML_NAMED_ELEMENT(MorphTarget)
    QML_ADDED_IN_VERSION(6, 0)

public:
    enum class MorphTargetAttribute {
        Position   = 0x01,
        Normal     = 0x02,
        Tangent    = 0x04,
        Binormal   = 0x08
    };
    Q_ENUM(MorphTargetAttribute)
    Q_DECLARE_FLAGS(MorphTargetAttributes , MorphTargetAttribute)

    explicit QQuick3DMorphTarget(QQuick3DObject *parent = nullptr);
    ~QQuick3DMorphTarget() override;

    float weight() const;
    MorphTargetAttributes attributes() const;

public Q_SLOTS:
    void setWeight(float castsShadows);
    void setAttributes(QQuick3DMorphTarget::MorphTargetAttributes attributes);
Q_SIGNALS:
    void weightChanged();
    void attributesChanged();

private:
    friend QQuick3DModel;
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void markAllDirty() override;
    size_t numAttribs();

    enum QSSGMorphTargetDirtyType {
        WeightDirty =                   0x00000001,
        MorphTargetAttributesDirty =    0x00000002,
    };

    quint32 m_dirtyAttributes = 0xffffffff; // all dirty by default
    void markDirty(QSSGMorphTargetDirtyType type);

    float m_weight = 0.0;
    MorphTargetAttributes m_attributes = MorphTargetAttribute::Position;
    size_t m_numAttribs = 1;
};

QT_END_NAMESPACE

#endif // QSSGMORPHTARGET_H
