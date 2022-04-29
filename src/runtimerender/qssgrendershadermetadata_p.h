/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
******************************************************************************/

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

    static Condition conditionFromString(const QString &condition);
};

struct InputOutput
{
    QByteArray type;
    QSSGShaderGeneratorStage stage = QSSGShaderGeneratorStage::Vertex;
    QByteArray name;

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
