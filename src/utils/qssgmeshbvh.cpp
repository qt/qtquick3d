// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qssgmeshbvh_p.h"

QT_BEGIN_NAMESPACE

QSSGMeshBVH::~QSSGMeshBVH()
{
    qDeleteAll(triangles);
    qDeleteAll(roots);
}

QT_END_NAMESPACE
