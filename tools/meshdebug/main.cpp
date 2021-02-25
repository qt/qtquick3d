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

#include <QtCore/QCoreApplication>
#include <QtCore/QCommandLineParser>
#include <QtCore/QFile>
#include <QtCore/QDebug>

#include <QtQuick3DUtils/private/qssgmesh_p.h>

using Qt::hex;

using namespace QSSGMesh;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Setup command line arguments
    QCommandLineParser cmdLineParser;
    cmdLineParser.process(app);
    QStringList meshFileNames = cmdLineParser.positionalArguments();

    // if there is nothing to do return early
    if (meshFileNames.isEmpty())
        return -1;

    // Process Mesh files
    for (const auto &meshFileName : meshFileNames) {
        QFile meshFile(meshFileName);
        if (!meshFile.open(QIODevice::ReadOnly)) {
            qWarning() << "could not open" << meshFileName;
            continue;
        }

        MeshInternal::MultiMeshInfo multiHeader = MeshInternal::readFileHeader(&meshFile);
        if (multiHeader.isValid()) {
            // Print Multiheader information
            qDebug() << " -- Multiheader --";
            qDebug() << "fileId:" << multiHeader.fileId;
            qDebug() << "version:" << multiHeader.fileVersion;
            qDebug() << "mesh entries:" << multiHeader.meshEntries.count();

            for (auto it = multiHeader.meshEntries.cbegin(), end = multiHeader.meshEntries.cend(); it != end; ++it) {
                const quint32 meshId = it.key();
                const quint64 meshOffset = it.value();
                qDebug() << "\t -- mesh entry --";
                qDebug() << "\tid:" << meshId;
                qDebug() << "\toffset:" << Qt::hex << meshOffset;

                MeshInternal::MeshDataHeader header;
                Mesh mesh;
                if (!MeshInternal::readMeshData(&meshFile, meshOffset, &mesh, &header)) {
                    qWarning("Failed to load mesh body");
                    continue;
                }

                // Header
                qDebug() << "\t -- MeshDataHeader --";
                qDebug() << "\tfileId:" << header.fileId;
                qDebug() << "\tfileVersion:" << header.fileVersion;
                qDebug() << "\tflags:" << header.flags;
                qDebug() << "\tsize in bytes:" << header.sizeInBytes;

                // Draw Mode
                qDebug() << "\t\tdraw mode:" << static_cast<int>(mesh.drawMode());

                // Winding
                qDebug() << "\t\twinding:" << toString(QSSGRenderWinding(mesh.winding()));

                // Vertex Buffer
                const Mesh::VertexBuffer vb = mesh.vertexBuffer();
                qDebug() << "\t\t -- Vertex Buffer --";
                qDebug() << "\t\tentry count:" << vb.entries.count();
                for (quint32 idx = 0, end = vb.entries.count(); idx < end; ++idx) {
                    qDebug() << "\t\t\t -- Vertex Buffer Entry" << idx << "--";
                    const Mesh::VertexBufferEntry &entry(vb.entries[idx]);
                    qDebug() << "\t\t\tname:" << entry.name;
                    qDebug() << "\t\t\ttype:" << toString(QSSGRenderComponentType(entry.componentType));
                    qDebug() << "\t\t\tcomponentCount:" << entry.componentCount;
                    qDebug() << "\t\t\tstart offset:" << entry.offset;
                }
                qDebug() << "\t\tstride:" << vb.stride;
                qDebug() << "\t\tdata size in bytes:" << vb.data.size();

                // Index Buffer
                const Mesh::IndexBuffer ib = mesh.indexBuffer();
                qDebug() << "\t\t -- Index Buffer --";
                qDebug() << "\t\tcomponentType:" << toString(QSSGRenderComponentType(ib.componentType));
                qDebug() << "\t\tdata size in bytes:" << ib.data.size();

                // Subsets
                const QVector<Mesh::Subset> subsets = mesh.subsets();
                qDebug() << "\t\t -- Subsets --";
                qDebug() << "\t\tsubset count:" << subsets.count();
                for (quint32 idx = 0, end = subsets.count(); idx < end; ++idx) {
                    qDebug() << "\t\t -- Subset" << idx << "--";
                    const Mesh::Subset &subset(subsets[idx]);
                    qDebug() << "\t\tindex count:" << subset.count;
                    qDebug() << "\t\tstart offset in indices:" << subset.offset;
                    qDebug() << "\t\tbounds: (" <<
                                subset.bounds.min.x() << "," <<
                                subset.bounds.min.y() << "," <<
                                subset.bounds.min.z() << ") (" <<
                                subset.bounds.max.x() << "," <<
                                subset.bounds.max.y() << "," <<
                                subset.bounds.max.z() << ")";
                    qDebug() << "\t\tname:" << subset.name;
                    if (header.hasLightmapSizeHint())
                        qDebug() << "\t\tlightmap size hint:" << subset.lightmapSizeHint;
                }
            }

        }

        meshFile.close();
        qDebug() << "closed meshFile";
    }

    return 0;
}
