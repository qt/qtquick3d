/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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


#include <QtTest>
#include <QDebug>
#include <QtQuick3DAssetImport/private/qssgassetimportmanager_p.h>
#include <QDir>
#include <QByteArray>

// add necessary includes here

class tst_assetimport : public QObject
{
    Q_OBJECT

public:
    tst_assetimport();
    ~tst_assetimport();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void importFile_data();
    void importFile();

};

tst_assetimport::tst_assetimport()
{

}

tst_assetimport::~tst_assetimport()
{

}

void tst_assetimport::initTestCase()
{

}

void tst_assetimport::cleanupTestCase()
{

}

void tst_assetimport::importFile_data()
{
    QTest::addColumn<QString>("extension");
    QTest::addColumn<bool>("result");
    QTest::addColumn<QByteArray>("expectedHash");

#ifdef __linux__
    QTest::newRow("fbx") << QString("fbx") << true << QByteArray("7b89122509ae5a7d86d749083c7f95e8");
    QTest::newRow("dae") << QString("dae") << true << QByteArray("28ac67e113d30797e6c4b1d9db68e0b0");
    QTest::newRow("obj") << QString("obj") << true << QByteArray("1dec3ac86c87ada356e1725aa73e66bd");
    QTest::newRow("blend") << QString("blend") << true << QByteArray("1ba292fb957d1beac3f3b4e62f0afff5");
    QTest::newRow("gltf") << QString("gltf") << true << QByteArray("3d66f10412546f5bc2993d8278f19093");
    QTest::newRow("glb") << QString("glb") << true << QByteArray("3d66f10412546f5bc2993d8278f19093");
#elif _WIN32
    QTest::newRow("fbx") << QString("fbx") << true << QByteArray("354564bca4d704245b239a1c7e4b08c8");
    QTest::newRow("dae") << QString("dae") << true << QByteArray("28ac67e113d30797e6c4b1d9db68e0b0");
    QTest::newRow("obj") << QString("obj") << true << QByteArray("de4ccf172258b7a7a419fc90980fa6db");
    QTest::newRow("blend") << QString("blend") << true << QByteArray("77c4703c3eddb004c4ab4b2968046b64");
    QTest::newRow("gltf") << QString("gltf") << true << QByteArray("3d66f10412546f5bc2993d8278f19093");
    QTest::newRow("glb") << QString("glb") << true << QByteArray("3d66f10412546f5bc2993d8278f19093");
#else
    QSKIP("Test not configured for this platform.");
#endif
}

void tst_assetimport::importFile()
{
    QFETCH(QString, extension);
    QFETCH(bool, result);
    QFETCH(QByteArray, expectedHash);

    QSSGAssetImportManager importManager;
    QString file = "resources/cube_scene." + extension;
    QString error;
    QByteArray fileChecksum;

    // Should return "true" if there were no errors opening the source or creating the exported object.
    auto importState = importManager.importFile(QFINDTESTDATA(file), QDir("./"), &error);
    const bool realResult = (importState == QSSGAssetImportManager::ImportState::Success);
    if(!error.isEmpty()) {
        if (importState == QSSGAssetImportManager::ImportState::Unsupported) {
            QEXPECT_FAIL("", "Unsupported format!", Continue);
        } else {
            qDebug() << "Error message:" << error;
            QFAIL(error.toStdString().c_str());
        }
    } else {
        // Generate a file hash of the created QML export to verify it was created correctly.
        // Returns empty QByteArray() on failure.
        QFile f("Cube_scene.qml");
        if (f.open(QFile::ReadOnly)) {
            QCryptographicHash hash(QCryptographicHash::Md5);
            if (hash.addData(&f)) {
                fileChecksum = hash.result();
            }
        }

        QCOMPARE(fileChecksum.toHex(), expectedHash);
    }

    QCOMPARE(realResult, result);
}

QTEST_APPLESS_MAIN(tst_assetimport)

#include "tst_assetimport.moc"
