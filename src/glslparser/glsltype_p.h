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

#ifndef QSSG_GLSLTYPE_H
#define QSSG_GLSLTYPE_H

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

#include <QtQuick3DGlslParser/private/glsl_p.h>

QT_BEGIN_NAMESPACE

namespace GLSL {

class Q_QUICK3DGLSLPARSER_EXPORT Type
{
public:
    virtual ~Type();

    virtual QString toString() const = 0;

    virtual const UndefinedType *asUndefinedType() const { return nullptr; }
    virtual const VoidType *asVoidType() const { return nullptr; }
    virtual const BoolType *asBoolType() const { return nullptr; }
    virtual const IntType *asIntType() const { return nullptr; }
    virtual const UIntType *asUIntType() const { return nullptr; }
    virtual const FloatType *asFloatType() const { return nullptr; }
    virtual const DoubleType *asDoubleType() const { return nullptr; }
    virtual const ScalarType *asScalarType() const { return nullptr; }
    virtual const IndexType *asIndexType() const { return nullptr; }
    virtual const VectorType *asVectorType() const { return nullptr; }
    virtual const MatrixType *asMatrixType() const { return nullptr; }
    virtual const ArrayType *asArrayType() const { return nullptr; }
    virtual const SamplerType *asSamplerType() const { return nullptr; }
    virtual const OverloadSet *asOverloadSetType() const { return nullptr; }

    virtual const Struct *asStructType() const { return nullptr; }
    virtual const Function *asFunctionType() const { return nullptr; }

    virtual bool isEqualTo(const Type *other) const = 0;
    virtual bool isLessThan(const Type *other) const = 0;
};

} // namespace GLSL

QT_END_NAMESPACE

#endif // QSSG_GLSLTYPE_H
