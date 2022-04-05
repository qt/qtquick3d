/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#ifndef QSSGLIGHTMAPPER_P_H
#define QSSGLIGHTMAPPER_P_H

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

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>

QT_BEGIN_NAMESPACE

struct QSSGLightmapperPrivate;
struct QSSGBakedLightingModel;
class QSSGRhiContext;
class QSSGRenderer;
struct QSSGRenderModel;

struct QSSGLightmapperOptions
{
    float opacityThreshold = 0.5f;
    float bias = 0.005f;
    bool useAdaptiveBias = true;
    bool indirectLightEnabled = true;
    int indirectLightSamples = 256;
    int indirectLightWorkgroupSize = 32;
    int indirectLightBounces = 3;
    float indirectLightFactor = 1.0f;
};

class QSSGLightmapper
{
public:
    QSSGLightmapper(QSSGRhiContext *rhiCtx, QSSGRenderer *renderer);
    ~QSSGLightmapper();
    void reset();
    void setOptions(const QSSGLightmapperOptions &options);
    qsizetype add(const QSSGBakedLightingModel &model);
    bool bake();

    static QString lightmapFilePathForLoad(const QSSGRenderModel &model);
    static QString lightmapFilePathForSave(const QSSGRenderModel &model);
    static QString lightmapListFilePath();

private:
    QSSGLightmapperPrivate *d = nullptr;
};

QT_END_NAMESPACE

#endif
