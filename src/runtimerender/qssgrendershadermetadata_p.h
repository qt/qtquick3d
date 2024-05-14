// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGRENDERSHADERMETADATA_P_H
#define QSSGRENDERSHADERMETADATA_P_H

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

#include <QtCore/qbytearray.h>
#include <QtCore/qvector.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>

namespace QSSGRenderShaderMetadata {

struct Uniform
{
    enum Condition {
        None,
        Regular,
        Negated
    };

    QByteArray type;
    Condition condition = Condition::None;
    QByteArray name;
    QByteArray conditionName;
    bool multiview = false;

    static Condition conditionFromString(const QString &condition);
};

struct InputOutput
{
    QByteArray type;
    QSSGShaderGeneratorStage stage = QSSGShaderGeneratorStage::Vertex;
    QByteArray name;
    bool flat = false;

    static QSSGShaderGeneratorStage stageFromString(const QString &stage);
};

struct ShaderMetaData
{
    QVector<Uniform> uniforms;
    QVector<InputOutput> inputs;
    QVector<InputOutput> outputs;
};

ShaderMetaData getShaderMetaData(const QByteArray &data);

} // namespace

#endif // QSSGRENDERSHADERMETADATA_H
