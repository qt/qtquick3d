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
class QDir;

namespace TypeInfo {
enum QmlType
{
    View3D,
    SceneEnvironment,
    PrincipledMaterial,
    DefaultMaterial,
    CustomMaterial,
    DirectionalLight,
    PointLight,
    SpotLight,
    Texture,
    TextureInput,
    Model,
    Builtin,
    Unknown
};

enum BuiltinType {
    Var = QMetaType::User,
    Int = QMetaType::Int,
    Bool = QMetaType::Bool,
    Real = QMetaType::Double,
    String = QMetaType::QString,
    Url = QMetaType::QUrl,
    Color = QMetaType::QColor,
    Font = QMetaType::QFont,
    Time = QMetaType::QTime,
    Date = QMetaType::QDate,
    DateTime = QMetaType::QDateTime,
    Rect = QMetaType::QRectF,
    Point = QMetaType::QPointF,
    Size = QMetaType::QSizeF,
    Vector2D = QMetaType::QVector2D,
    Vector3D = QMetaType::QVector3D,
    Vector4D = QMetaType::QVector4D,
    Matrix4x4 = QMetaType::QMatrix4x4,
    Quaternion = QMetaType::QQuaternion,
    InvalidBuiltin = QMetaType::UnknownType
};
}

namespace MaterialParser {

struct Light
{
    QQuick3DAbstractLight *ptr = nullptr;
    TypeInfo::QmlType type;
};

struct Material
{
    QQuick3DMaterial *ptr = nullptr;
    TypeInfo::QmlType type;
};

inline bool operator==(const Material &l, const Material &r) { return (l.ptr == r.ptr); }

struct SceneData
{
    QQuick3DViewport *viewport = nullptr; // NOTE!!! we're only handling one viewport atm.
    QVector<Light> lights;
    QVector<Material> materials;
    QVector<QQuick3DTexture *> textures;
    QVector<QQuick3DModel *> models;
    bool hasData() { return viewport && (models.size() != 0 || materials.size() != 0); }
};

int parseQmlData(const QByteArray &code, const QString &fileName, SceneData &sceneData);
int parseQmlFiles(const QVector<QString> &filePaths, const QDir &sourceDir, SceneData &sceneData, bool verboseOutput);

}

Q_DECLARE_TYPEINFO(MaterialParser::Light, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif // PARSER_H
