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
#include <QtCore/QDebug>

#include <QtQuick3DAssetImport/private/qssgmeshutilities_p.h>

#include <QtGui/QMatrix4x4>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
// QTextStream functions are moved to a namespace in Qt6
using Qt::hex;
#endif

using namespace QSSGMeshUtilities;

class MeshOffsetTracker
{
public:
    MeshOffsetTracker(qint64 startOffset)
        : m_startOffset(startOffset)
    {
    }

    bool needsAlignment() { return getAlignmentAmount() > 0; }
    quint32 getAlignmentAmount() { return 4 - (m_byteCounter % 4); }
    void align()
    {
        if (needsAlignment())
            m_byteCounter += getAlignmentAmount();
    }

    void advance(qint64 offset, bool forceAlign = true) {
        m_byteCounter += offset;
        if (forceAlign)
            align();
    }

    template <typename TDataType>
    void advance(OffsetDataRef<TDataType> &data, bool forceAlign = true)
    {
        data.m_offset = m_byteCounter;
        m_byteCounter += data.size() * sizeof(TDataType);
        if (forceAlign)
            align();
    }

    qint64 startOffset() const
    {
        return m_startOffset;
    }

    qint64 offset() const
    {
        return m_startOffset + m_byteCounter;
    }

private:
    qint64 m_startOffset = 0;
    qint64 m_byteCounter = 0;
};


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
            qWarning() << "could not open " << meshFileName;
            continue;
        }

        if (Mesh::isMulti(meshFile)) {
            MeshMultiHeader* multiHeader = Mesh::loadMultiHeader(meshFile);
            if (!multiHeader) {
                qWarning() << "could not read MeshMultiHeader in " << meshFileName;
                continue;
            }

            // Print Multiheader information
            qDebug() << " -- Multiheader -- ";
            qDebug() << "fileId: " << multiHeader->m_fileId;
            qDebug() << "version: " << multiHeader->m_version;
            qDebug() << "mesh entries: " << multiHeader->m_entries.m_size;

            quint8 *theHeaderBaseAddr = reinterpret_cast<quint8 *>(multiHeader);
            bool foundMesh = false;
            for (quint32 idx = 0, end = multiHeader->m_entries.size(); idx < end && !foundMesh; ++idx) {
                const MeshMultiEntry &entry(multiHeader->m_entries.index(theHeaderBaseAddr, idx));
                qDebug() << "\t -- mesh entry" << idx << " -- ";
                qDebug() << "\tid: " << entry.m_meshId;
                qDebug() << "\toffset: " << Qt::hex << entry.m_meshOffset;
                qDebug() << "\tpadding: " << entry.m_padding;

                meshFile.seek(entry.m_meshOffset);

                // Read mesh header
                MeshDataHeader header;
                meshFile.read(reinterpret_cast<char *>(&header), sizeof(MeshDataHeader));
                qDebug() << "\t -- MeshDataHeader -- ";
                qDebug() << "\tfileId: " << header.m_fileId;
                qDebug() << "\tfileVersion: " << header.m_fileVersion;
                qDebug() << "\tflags: " << header.m_headerFlags;
                qDebug() << "\tsize(in bytes): " << header.m_sizeInBytes;

                QByteArray meshMetadataData = meshFile.read(sizeof(Mesh));
                Mesh *mesh = reinterpret_cast<Mesh *>(meshMetadataData.data());
                MeshOffsetTracker offsetTracker(entry.m_meshOffset + sizeof(MeshDataHeader));
                offsetTracker.advance(sizeof(Mesh), false);

                // Vertex Buffer
                qDebug() << "\t\t -- Vertex Buffer --";
                meshFile.seek(offsetTracker.offset());
                offsetTracker.advance(mesh->m_vertexBuffer.m_entries);
                qDebug() << "\t\tentries offset: " << Qt::hex << mesh->m_vertexBuffer.m_entries.m_offset;
                qDebug() << "\t\tentries size: " << mesh->m_vertexBuffer.m_entries.m_size * sizeof(MeshVertexBufferEntry);
                QByteArray vertexBufferEntriesData = meshFile.read(mesh->m_vertexBuffer.m_entries.m_size * sizeof(MeshVertexBufferEntry));
                for (quint32 idx = 0, end = mesh->m_vertexBuffer.m_entries.size(); idx < end; ++idx) {
                    // get name length
                    meshFile.seek(offsetTracker.offset());
                    QByteArray lenghtBuffer = meshFile.read(sizeof (quint32));
                    const quint32 &nameLenght = *reinterpret_cast<const quint32 *>(lenghtBuffer.constData());
                    offsetTracker.advance(sizeof(quint32), false);
                    QByteArray nameBuffer = meshFile.read(nameLenght);
                    offsetTracker.advance(nameLenght);
                    qDebug() << "\t\t\t -- Vertex Buffer Entry " << idx << "-- ";
                    const MeshVertexBufferEntry &entry = reinterpret_cast<const MeshVertexBufferEntry *>(vertexBufferEntriesData.constData())[idx];
                    qDebug() << "\t\t\tname: " << nameBuffer.constData();
                    qDebug() << "\t\t\ttype: " << toString(entry.m_componentType);
                    qDebug() << "\t\t\tnumComponents: " << entry.m_numComponents;
                    qDebug() << "\t\t\tfirstItemOffset: " << entry.m_firstItemOffset;
                }
                offsetTracker.advance(mesh->m_vertexBuffer.m_data);
                qDebug() << "\t\tstride: " << mesh->m_vertexBuffer.m_stride;
                qDebug() << "\t\tdata Offset: " << Qt::hex << mesh->m_vertexBuffer.m_data.m_offset;
                qDebug() << "\t\tdata Size: " << mesh->m_vertexBuffer.m_data.m_size * sizeof(quint8);

                // Index Buffer
                qDebug() << "\t\t -- Index Buffer -- ";
                offsetTracker.advance(mesh->m_indexBuffer.m_data);
                qDebug() << "\t\tcomponentType: " << toString(mesh->m_indexBuffer.m_componentType);
                qDebug() << "\t\tdata Offset: " << Qt::hex << mesh->m_indexBuffer.m_data.m_offset;
                qDebug() << "\t\tdata Size: " << mesh->m_indexBuffer.m_data.m_size * sizeof(quint8);

                // Subsets
                qDebug() << "\t\t -- Subsets -- ";
                meshFile.seek(offsetTracker.offset());
                offsetTracker.advance(mesh->m_subsets);
                qDebug() << "\t\toffset: " << Qt::hex << mesh->m_subsets.m_offset;
                qDebug() << "\t\tsize: " << mesh->m_subsets.m_size * sizeof(MeshSubset);
                QByteArray subsetEntriesData = meshFile.read(mesh->m_subsets.m_size * sizeof(MeshSubset));
                for (quint32 idx = 0, end = mesh->m_subsets.size(); idx < end; ++idx) {
                    qDebug() << "\t\t -- Subset " << idx << "-- ";
                    MeshSubset &subset = reinterpret_cast<MeshSubset *>(subsetEntriesData.data())[idx];
                    qDebug() << "\t\thasCount: " << subset.hasCount();
                    qDebug() << "\t\tcount: " << subset.m_count;
                    qDebug() << "\t\toffset(size): " << subset.m_offset;
                    qDebug() << "\t\tbounds: (" << subset.m_bounds.minimum.x() << "," <<
                                subset.m_bounds.minimum.y() << "," <<
                                subset.m_bounds.minimum.z() << ") (" <<
                                subset.m_bounds.maximum.x() << "," <<
                                subset.m_bounds.maximum.y() << "," <<
                                subset.m_bounds.maximum.z() << ")";
                    meshFile.seek(offsetTracker.offset());
                    offsetTracker.advance(subset.m_name);
                    qDebug() << "\t\tname offset: " << Qt::hex << subset.m_name.m_offset;
                    qDebug() << "\t\tname size: " << subset.m_name.m_size * sizeof(char16_t);
                    QByteArray subsetNameBuffer = meshFile.read(subset.m_name.m_size * sizeof(char16_t));
                    const char16_t* name = reinterpret_cast<const char16_t*>(subsetNameBuffer.constData());
                    qDebug() << "\t\tname: " << QString::fromUtf16(name);
                }

                // Joints
                qDebug() << "\t\t -- Joints -- ";
                meshFile.seek(offsetTracker.offset());
                offsetTracker.advance(mesh->m_joints);
                qDebug() << "\t\toffset: " << Qt::hex << mesh->m_joints.m_offset;
                qDebug() << "\t\tsize: " << mesh->m_joints.m_size * sizeof(Joint);
                QByteArray jointsData = meshFile.read(mesh->m_joints.m_size * sizeof(Joint));
                for (quint32 idx = 0, end = mesh->m_joints.size(); idx < end; ++idx) {
                    qDebug() << "\t\t -- Joint " << idx << "-- ";
                    const Joint &joint = reinterpret_cast<const Joint *>(jointsData.constData())[idx];
                    qDebug() << "\t\tid: " << joint.m_jointID;
                    qDebug() << "\t\tparentId: " << joint.m_parentID;
                    qDebug() << "\t\tinvBindPose: " << QMatrix4x4(joint.m_invBindPose);
                    qDebug() << "\t\tlocalToGlobalBoneSpace: " << QMatrix4x4(joint.m_localToGlobalBoneSpace);
                }

                // Draw Mode
                qDebug() << "\t\tdraw Mode: " << static_cast<int>(mesh->m_drawMode);

                // Winding
                qDebug() << "\t\twinding: " << toString(mesh->m_winding);

            }

        }

        meshFile.close();
        qDebug() << "closed meshFile";
    }

    return 0;
}
