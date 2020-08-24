/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
#include <QtQuick3DRuntimeRender/private/qssgrenderimagetexturedata_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>
#include <QtCore/qsize.h>
#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderTextureData : public QSSGRenderGraphObject
{
public:
    explicit QSSGRenderTextureData();
    virtual ~QSSGRenderTextureData();

    const QByteArray &textureData() const;
    void setTextureData(const QByteArray &data);

    QSize size() const;
    void setSize(const QSize &size);

    QSSGRenderTextureFormat format() const;
    void setFormat(QSSGRenderTextureFormat format);

    bool hasTransparancy() const;
    void setHasTransparency(bool hasTransparency);

    QSSGRenderImageTextureData createOrUpdate(const QSSGRef<QSSGBufferManager> &bufferManager,
                                              QSSGBufferManager::MipMode mipMode = QSSGBufferManager::MipModeNone);

protected:
    Q_DISABLE_COPY(QSSGRenderTextureData)

    QSSGRenderImageTextureData m_texture;
    QByteArray m_textureData;
    QSize m_size;
    QSSGRenderTextureFormat m_format = QSSGRenderTextureFormat::Unknown;
    bool m_dirty = true;
    bool m_hasTransparency = false;
};

QT_END_NAMESPACE

#endif // QSSGRENDERTEXTUREDATA_H
