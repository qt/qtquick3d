/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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


#ifndef QQUICK3DRESOURCELOADER_H
#define QQUICK3DRESOURCELOADER_H

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

#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dgeometry_p.h>
#include <QtQuick3D/private/qquick3dtexture_p.h>
#include <QtQml/QQmlListProperty>

QT_BEGIN_NAMESPACE

class Q_QUICK3D_EXPORT QQuick3DResourceLoader : public QQuick3DObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QUrl> meshSources READ meshSources WRITE setMeshSources NOTIFY meshSourcesChanged)
    Q_PROPERTY(QQmlListProperty<QQuick3DTexture> textures READ textures)
    Q_PROPERTY(QQmlListProperty<QQuick3DGeometry> geometries READ geometries)
    QML_NAMED_ELEMENT(ResourceLoader)
    QML_ADDED_IN_VERSION(6, 3)
public:
    QQuick3DResourceLoader(QQuick3DObject *parent = nullptr);

    const QList<QUrl> &meshSources() const;
    void setMeshSources(const QList<QUrl> &newMeshSources);
    QQmlListProperty<QQuick3DGeometry> geometries();
    QQmlListProperty<QQuick3DTexture> textures();
Q_SIGNALS:
    void meshSourcesChanged();

private Q_SLOTS:
    void onGeometryDestroyed(QObject *object);
    void onTextureDestroyed(QObject *object);
protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void markAllDirty() override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;

private:
    static void qmlAppendGeometry(QQmlListProperty<QQuick3DGeometry> *list, QQuick3DGeometry *geometry);
    static QQuick3DGeometry *qmlGeometryAt(QQmlListProperty<QQuick3DGeometry> *list, qsizetype index);
    static qsizetype qmlGeometriesCount(QQmlListProperty<QQuick3DGeometry> *list);
    static void qmlClearGeometries(QQmlListProperty<QQuick3DGeometry> *list);

    static void qmlAppendTexture(QQmlListProperty<QQuick3DTexture> *list, QQuick3DTexture *texture);
    static QQuick3DTexture *qmlTextureAt(QQmlListProperty<QQuick3DTexture> *list, qsizetype index);
    static qsizetype qmlTexturesCount(QQmlListProperty<QQuick3DTexture> *list);
    static void qmlClearTextures(QQmlListProperty<QQuick3DTexture> *list);

    enum ResourceLoaderDirtyType {
        MeshesDirty =       0x00000001,
        TexturesDirty =     0x00000002,
        GeometriesDirty =   0x00000004
    };


    quint32 m_dirtyAttributes = 0xffffffff; // all dirty by default
    void markDirty(ResourceLoaderDirtyType type);
    void updateSceneManager(QQuick3DSceneManager *sceneManager);

    QList<QUrl> m_meshSources;
    QList<QQuick3DGeometry *> m_geometries;
    QList<QQuick3DTexture *> m_textures;
};

QT_END_NAMESPACE

#endif // QQUICK3DRESOURCELOADER_H
