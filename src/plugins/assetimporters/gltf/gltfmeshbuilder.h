// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#pragma once

#include "gltfsceneconverter.h"

#include <QtQuick3DGltf/private/qssggltfdocument_p.h>
#include <QtQuick3DUtils/private/qssgmesh_p.h>

QT_BEGIN_NAMESPACE

namespace GltfMeshBuilder {

QSSGMesh::Mesh buildMesh(const QSSGGltfDocument &document, const QSSGGltf::Mesh &mesh,
                         const GltfSceneConverter::Options &options,
                         QList<int> *usedPrimitives = nullptr,
                         GltfMorphTargetInfo *morphInfo = nullptr);

} // namespace GltfMeshBuilder

QT_END_NAMESPACE
