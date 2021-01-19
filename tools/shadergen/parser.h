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
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class QQuick3DSceneEnvironment;
class QQuick3DAbstractLight;
class QQuick3DMaterial;
class QQuick3DViewport;
class QQuick3DTexture;
class QQuick3DModel;
class QQuick3DEffect;
class QQuick3DShaderUtilsShader;
class QDir;

namespace MaterialParser {

struct SceneData
{
    QQuick3DViewport *viewport = nullptr; // NOTE!!! we're only handling one viewport atm.
    QVector<QQuick3DAbstractLight *> lights;
    QVector<QQuick3DMaterial *> materials;
    QVector<QQuick3DTexture *> textures;
    QVector<QQuick3DModel *> models;
    QVector<QQuick3DEffect *> effects;
    QVector<QQuick3DShaderUtilsShader *> shaders;
    bool hasData() const { return viewport && (models.size() != 0 || materials.size() != 0 || effects.size() != 0); }
};

int parseQmlData(const QByteArray &code, const QString &fileName, SceneData &sceneData);
int parseQmlFiles(const QVector<QString> &filePaths, const QDir &sourceDir, SceneData &sceneData, bool verboseOutput);

}

QT_END_NAMESPACE

#endif // PARSER_H
