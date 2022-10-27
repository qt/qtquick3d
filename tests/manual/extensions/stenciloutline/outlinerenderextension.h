// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef OUTLINERENDEREXTENSION_H
#define OUTLINERENDEREXTENSION_H

#include <QtQuick3D/private/qquick3drenderextensions_p.h>
#include <QtQmlIntegration>

class OutlineRenderExtension : public QQuick3DRenderExtension
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DObject * target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QQuick3DObject * outlineMaterial READ outlineMaterial WRITE setOutlineMaterial NOTIFY outlineMaterialChanged)
    Q_PROPERTY(float outlineScale READ outlineScale WRITE setOutlineScale NOTIFY outlineScaleChanged)
    QML_ELEMENT

public:
    OutlineRenderExtension() = default;
    ~OutlineRenderExtension() override;

    float outlineScale() const;
    void setOutlineScale(float newOutlineScale);

    QQuick3DObject *target() const;
    void setTarget(QQuick3DObject *newTarget);

    QQuick3DObject *outlineMaterial() const;
    void setOutlineMaterial(QQuick3DObject *newOutlineMaterial);

signals:
    void outlineColorChanged();
    void outlineScaleChanged();
    void targetChanged();

    void outlineMaterialChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;

private:
    enum Dirty : quint8
    {
        Target = 0x1,
        OutlineMaterial = 0x2,
        OutlineScale = 0x3,
    };

    using DirtyT = std::underlying_type_t<Dirty>;

    void markDirty(Dirty v);

    QQuick3DObject *m_target = nullptr;
    QQuick3DObject *m_outlineMaterial = nullptr;
    float m_outlineScale = 1.05f;
    DirtyT m_dirtyFlag {};
};

#endif // OUTLINERENDEREXTENSION_H
