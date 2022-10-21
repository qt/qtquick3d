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

#include <QtQuick3D/private/qquick3dinstancing_p.h>

extern void qt_writeInstanceTable(QIODevice *out, QQuick3DInstancing &instanceTable);

int main(int argc, char **argv)
{
    QQuick3DFileInstancing instanceTable;
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s INFILE [OUTFILE]\n", argv[0]);
        return -1;
    }
    QString inFilename = QString::fromLocal8Bit(argv[1]);
    if (!instanceTable.loadFromXmlFile(inFilename)) {
        fprintf(stderr, "Could not read instance table %s\n", argv[1]);
        return -2;
    }

    QString outFilename = argc > 2 ? QString::fromLocal8Bit(argv[2]) : inFilename + QStringLiteral(".bin");
    QFile outFile(outFilename);
    if (!outFile.open(QFile::WriteOnly)) {
        fprintf(stderr, "Could not open %s for writing.\n", qPrintable(outFilename));
        return -2;
    }

    int instanceCount = instanceTable.writeToBinaryFile(&outFile);

    outFile.close();

    if (instanceCount < 0) {
        fprintf(stderr, "Could not write instance table.\n");
        return -3;
    }

    fprintf(stderr, "Wrote %d instances to %s.\n", instanceCount, qPrintable(outFilename));

    return 0;
}
