// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QQUICK3DSHADERIMAGE_P_H
#define QQUICK3DSHADERIMAGE_P_H

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

#include <QtCore/qstring.h>
#include <QtQuick3D/qquick3dobject.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3D_EXPORT QQuick3DShaderImage : public QQuick3DObject
{
    Q_OBJECT
    Q_PROPERTY(ImageShape shape READ shape WRITE setShape NOTIFY shapeChanged FINAL)
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged FINAL)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged FINAL)
    Q_PROPERTY(int depth READ depth WRITE setDepth NOTIFY depthChanged FINAL)
    Q_PROPERTY(int arraySize READ arraySize WRITE setArraySize NOTIFY arraySizeChanged FINAL)
    Q_PROPERTY(ImageFormat format READ format WRITE setFormat NOTIFY formatChanged FINAL)
    QML_NAMED_ELEMENT(ShaderImage)
    QML_ADDED_IN_VERSION(6, 13)

public:
    enum ImageShape {
        Image1D,
        Image2D,
        Image3D,
        Image2DArray,
        ImageCube,
    };
    Q_ENUM(ImageShape)

    enum ImageFormat {
        RGBA8,
        R8,
        RG8,
        R16,
        RG16,
        RGBA16F,
        RGBA32F,
        R16F,
        R32F,
        RGB10A2,
        R8SI,
        R32SI,
        RG32SI,
        RGBA32SI,
        R8UI,
        R32UI,
        RG32UI,
        RGBA32UI,
    };
    Q_ENUM(ImageFormat);
    explicit QQuick3DShaderImage(QQuick3DObject *parent = nullptr);
    ~QQuick3DShaderImage() override;

    QQuick3DShaderImage::ImageShape shape() const;
    int width() const;
    int height() const;
    int depth() const;
    int arraySize() const;
    QQuick3DShaderImage::ImageFormat format() const;

public Q_SLOTS:
    void setShape(QQuick3DShaderImage::ImageShape shape);
    void setWidth(int width);
    void setHeight(int height);
    void setDepth(int depth);
    void setArraySize(int arraySize);
    void setFormat(QQuick3DShaderImage::ImageFormat format);

Q_SIGNALS:
    void shapeChanged();
    void widthChanged();
    void heightChanged();
    void depthChanged();
    void arraySizeChanged();
    void formatChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void markAllDirty() override;
    void markSizeDirty();

private:
    enum class DirtyFlag {
        SizeDirty = 1,
        FormatDirty = 2,
    };
    Q_DECLARE_FLAGS(DirtyFlags, DirtyFlag)
    DirtyFlags m_dirtyFlags = DirtyFlags(DirtyFlag::SizeDirty) | DirtyFlags(DirtyFlag::FormatDirty);
    QQuick3DShaderImage::ImageShape m_shape = ImageShape::Image2D;
    int m_width = 1;
    int m_height = 1;
    int m_depth = 1;
    int m_arraySize = 1;
    QQuick3DShaderImage::ImageFormat m_format = ImageFormat::RGBA8;
};

QT_END_NAMESPACE

#endif // QQUICK3DSHADERIMAGE_P_H
