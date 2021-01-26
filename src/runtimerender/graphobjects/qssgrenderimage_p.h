/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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

#ifndef QSSG_RENDER_IMAGE_H
#define QSSG_RENDER_IMAGE_H

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

#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimagetexturedata_p.h>
#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <QtQuick3DRender/private/qssgrendertexture2d_p.h>

#include <QtGui/QVector2D>

QT_BEGIN_NAMESPACE
class QSSGRenderContextInterface;
class QSGTexture;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderImage : public QSSGRenderGraphObject
{
    enum class Flag
    {
        Dirty = 1,
        TransformDirty = 1 << 1,
        Active = 1 << 2, ///< Is this exact object active
        ItemSizeDirty = 1 << 3
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    enum class MappingModes : quint8
    {
        Normal = 0, // UV mapping
        Environment = 1,
        LightProbe = 2,
    };

    Q_DISABLE_COPY(QSSGRenderImage)
    // Complete path to the file;
    //*not* relative to the presentation directory
    QString m_imagePath;
    QString m_imageShaderName; ///< for custom materials we don't generate the name

    QSSGRenderGraphObject *m_parent = nullptr;

    QSSGRenderImageTextureData m_textureData;

    QSGTexture *m_qsgTexture = nullptr; // overrides source if available

    Flags m_flags; // only dirty, transform dirty, and active apply

    QVector2D m_scale { 1.0f, 1.0f };
    QVector2D m_pivot { 0.0f, 0.0f };
    QVector2D m_position { 0.0f, 0.0f };
    float m_rotation = 0.0f; // Radians.
    bool m_flipV = false;
    MappingModes m_mappingMode = MappingModes::Normal;
    QSSGRenderTextureCoordOp m_horizontalTilingMode = QSSGRenderTextureCoordOp::ClampToEdge;
    QSSGRenderTextureCoordOp m_verticalTilingMode = QSSGRenderTextureCoordOp::ClampToEdge;
    QSSGRenderTextureFormat m_format = QSSGRenderTextureFormat::Unknown;

    // Setting any of the above variables means this object is dirty.
    // Setting any of the vec2 properties means this object's transform is dirty
    QMatrix4x4 m_textureTransform;

    QSSGRenderImage();
    ~QSSGRenderImage();
    // Renders the sub presentation
    // Or finds the image.
    // and sets up the texture transform
    bool clearDirty(const QSSGRef<QSSGBufferManager> &inBufferManager, bool forIbl = false);

    void calculateTextureTransform();

    bool isImageTransformIdentity() const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSSGRenderImage::Flags)

QT_END_NAMESPACE

#endif
