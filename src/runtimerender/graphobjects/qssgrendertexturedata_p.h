// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGRENDERTEXTUREDATA_H
#define QSSGRENDERTEXTUREDATA_H

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
#include <QtQuick3DRuntimeRender/private/qssgrenderimagetexture_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>
#include <QtCore/qsize.h>
#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderTextureData : public QSSGRenderGraphObject
{
public:
    explicit QSSGRenderTextureData();
    // it's specially used for Type::Skin
    explicit QSSGRenderTextureData(QSSGRenderGraphObject::Type inType);
    virtual ~QSSGRenderTextureData();

    const QByteArray &textureData() const;
    void setTextureData(const QByteArray &data);

    QSize size() const;
    void setSize(const QSize &size);

    int depth() const;
    void setDepth(int depth);

    QSSGRenderTextureFormat format() const;
    void setFormat(QSSGRenderTextureFormat format);

    bool hasTransparancy() const;
    void setHasTransparency(bool hasTransparency);

    uint32_t generationId() const;

    QString debugObjectName;

protected:
    Q_DISABLE_COPY(QSSGRenderTextureData)

    void markDirty();

    QByteArray m_textureData;
    QSize m_size;
    int m_depth = 0;
    QSSGRenderTextureFormat m_format = QSSGRenderTextureFormat::Unknown;
    bool m_hasTransparency = false;
    uint32_t m_generationId = 1;
};

QT_END_NAMESPACE

#endif // QSSGRENDERTEXTUREDATA_H
