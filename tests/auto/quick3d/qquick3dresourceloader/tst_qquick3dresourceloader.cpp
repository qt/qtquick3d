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

#include <QTest>
#include <QSignalSpy>

#include <QtQuick3D/private/qquick3dresourceloader_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderresourceloader_p.h>
#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

class tst_QQuick3DResourceLoader : public QObject
{
    Q_OBJECT

    // Work-around to get access to updateSpatialNode
    class ResourceLoader : public QQuick3DResourceLoader
    {
    public:
        using QQuick3DResourceLoader::updateSpatialNode;
    };

private Q_SLOTS:
    void testProperties();

};

void tst_QQuick3DResourceLoader::testProperties()
{
    ResourceLoader resourceLoader;
    auto node = static_cast<QSSGRenderResourceLoader *>(resourceLoader.updateSpatialNode(nullptr));
    QVERIFY(node);

    // We can really only test the meshSources property as the other two properties are
    // QML Lists instead of value based lists.

    // Mesh Sources
    QSignalSpy spy(&resourceLoader, SIGNAL(meshSourcesChanged()));
    resourceLoader.setMeshSources( {QUrl("#Cube"), QUrl("#Sphere")} );
    QCOMPARE(spy.count(), 1);
    node = static_cast<QSSGRenderResourceLoader *>(resourceLoader.updateSpatialNode(nullptr));
    QCOMPARE(resourceLoader.meshSources().count(), 2);
    QCOMPARE(node->meshes.count(), 2);

}

QTEST_APPLESS_MAIN(tst_QQuick3DResourceLoader)
#include "tst_qquick3dresourceloader.moc"
