// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuick3DUtils/private/qssgmesh_p.h>

#include <QtCore/QBuffer>
#include <QtCore/QByteArray>

// At fuzzing rates the warnings alone would dominate the run time.
static QtMessageHandler mh = qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &) { });

extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size)
{
    // No QGuiApplication: nothing on the load path needs one, and if that ever
    // stops being true the reader has grown a dependency it should not have.
    QByteArray blob = QByteArray::fromRawData(Data, qsizetype(Size));
    QBuffer buffer(&blob);
    if (!buffer.open(QIODevice::ReadOnly))
        return 0;

    // loadAll() rather than loadMesh() so that the container entry list is
    // exercised too, not just the first mesh.
    const QMap<quint32, QSSGMesh::Mesh> meshes = QSSGMesh::Mesh::loadAll(&buffer);
    for (const QSSGMesh::Mesh &mesh : meshes) {
        for (const QSSGMesh::Mesh::Subset &subset : mesh.subsets())
            (void)subset.name.size();
    }
    return 0;
}
