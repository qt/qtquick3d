/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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

#ifndef QSSGSHADERMAPKEY_P_H
#define QSSGSHADERMAPKEY_P_H

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

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>

struct QSSGShaderMapKey
{
    QByteArray m_name;
    const ShaderFeatureSetList *m_featuresOrig;
    ShaderFeatureSetList m_featuresCopy;
    QSSGShaderDefaultMaterialKey *m_materialKeyOrig;
    QSSGShaderDefaultMaterialKey m_materialKeyCopy;
    size_t m_hashCode;

    void detach()
    {
        if (m_featuresOrig) {
            m_featuresCopy = *m_featuresOrig;
            m_featuresOrig = nullptr;
        }
        if (m_materialKeyOrig) {
            m_materialKeyCopy = *m_materialKeyOrig;
            m_materialKeyOrig = nullptr;
        }
    }

    QSSGShaderMapKey(const QByteArray &inName,
                     const ShaderFeatureSetList &inFeatures,
                     QSSGShaderDefaultMaterialKey &inMaterialKey)
        : m_name(inName), m_featuresOrig(&inFeatures), m_materialKeyOrig(&inMaterialKey)
    {
        m_hashCode = qHash(m_name) ^ hashShaderFeatureSet(*m_featuresOrig) ^ qHash(m_materialKeyOrig->hash());
    }
};

inline bool operator==(const QSSGShaderMapKey &a, const QSSGShaderMapKey &b) Q_DECL_NOTHROW
{
    if (a.m_name != b.m_name)
        return false;

    const QSSGShaderDefaultMaterialKey *keyA = a.m_materialKeyOrig ? a.m_materialKeyOrig : &a.m_materialKeyCopy;
    const QSSGShaderDefaultMaterialKey *keyB = b.m_materialKeyOrig ? b.m_materialKeyOrig : &b.m_materialKeyCopy;
    if (!(*keyA == *keyB))
        return false;

    const ShaderFeatureSetList *featuresA = a.m_featuresOrig ? a.m_featuresOrig : &a.m_featuresCopy;
    const ShaderFeatureSetList *featuresB = b.m_featuresOrig ? b.m_featuresOrig : &b.m_featuresCopy;
    return *featuresA == *featuresB;
}

inline size_t qHash(const QSSGShaderMapKey &key, size_t seed)
{
    return key.m_hashCode ^ seed;
}

#endif
