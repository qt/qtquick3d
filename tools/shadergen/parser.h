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

#ifndef PARSER_H
#define PARSER_H

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

class QQuick3DSceneEnvironment;
class QQuick3DSpotLight;
class QQuick3DAreaLight;
class QQuick3DDirectionalLight;
class QQuick3DPointLight;
class QQuick3DPrincipledMaterial;
class QQuick3DViewport;
class QQuick3DTexture;
class QDir;

namespace MaterialParser {

struct SceneData
{
    QQuick3DViewport *viewport = nullptr; // NOTE!!! we're only handling one viewport atm.
    QVector<QQuick3DSpotLight *> spotLights;
    QVector<QQuick3DAreaLight *> areaLights;
    QVector<QQuick3DDirectionalLight *> directionalLights;
    QVector<QQuick3DPointLight *> pointLights;
    QVector<QQuick3DPrincipledMaterial *> materials;
    QVector<QQuick3DTexture *> textures;
    bool hasData() { return viewport && materials.size() != 0; }
};

int parseQmlFiles(const QVector<QStringRef> &filePaths, const QDir &sourceDir, SceneData &sceneData, bool verboseOutput);

}

QT_END_NAMESPACE

#endif // PARSER_H
