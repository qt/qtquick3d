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

#ifndef GENSHADERS_H
#define GENSHADERS_H

#include "parser.h"

#include <QtCore/qlist.h>

#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>

QT_BEGIN_NAMESPACE
class QDir;
class QQuick3DSceneManager;
QT_END_NAMESPACE

struct GenShaders
{
    explicit GenShaders();
    ~GenShaders();
    bool process(const MaterialParser::SceneData &sceneData, QVector<QString> &qsbcFiles, const QDir &outDir,
                 bool generateMultipleLights, bool dryRun);

    QRhi *rhi = nullptr;
    QSSGRef<QSSGRenderContextInterface> renderContext;

    QQuick3DSceneManager *sceneManager = nullptr;
};

#endif
