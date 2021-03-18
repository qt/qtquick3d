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

#include <QtQuick3DRender/private/qssgrenderconstantbuffer_p.h>
#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>

#include <QtQuick3DUtils/private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

uint qHash(const QSSGRenderConstantBuffer::ParamHandle &h, uint seed) Q_DECL_NOTHROW
{
    return (h.key) ? h.key : QT_PREPEND_NAMESPACE(qHash(h.name, seed));
}

///< struct handling a constant buffer entry
class ConstantBufferParamEntry
{
public:
    QByteArray m_name; ///< parameter Name
    QSSGRenderShaderDataType m_type; ///< parameter type
    qint32 m_count; ///< one or array size
    qint32 m_offset; ///< offset into the memory buffer

    ConstantBufferParamEntry(const QByteArray &name, QSSGRenderShaderDataType type, qint32 count, qint32 offset)
        : m_name(name), m_type(type), m_count(count), m_offset(offset)
    {
    }
};

QSSGRenderConstantBuffer::QSSGRenderConstantBuffer(const QSSGRef<QSSGRenderContext> &context,
                                                       const QByteArray &bufferName,
                                                       QSSGRenderBufferUsageType usageType,
                                                       QSSGByteView data)
    : QSSGRenderDataBuffer(context, QSSGRenderBufferType::Constant, usageType, data)
    , m_name(bufferName)
    , m_currentOffset(0)
    , m_currentSize(0)
    , m_hwBufferInitialized(false)
    , m_maxBlockSize(0)
{
    Q_ASSERT(context->supportsConstantBuffer());

    m_backend->getRenderBackendValue(QSSGRenderBackend::QSSGRenderBackendQuery::MaxConstantBufferBlockSize, &m_maxBlockSize);

    if (data.size()) {
        Q_ASSERT(data.size() < m_maxBlockSize);
        m_shadowCopy.resize(data.size());
        memcpy(m_shadowCopy.begin(), data.begin(), size_t(data.size()));
    }
    context->registerConstantBuffer(this);
}

QSSGRenderConstantBuffer::~QSSGRenderConstantBuffer()
{
    qDeleteAll(m_constantBufferEntryMap);
    m_context->bufferDestroyed(this);
}

void QSSGRenderConstantBuffer::bind()
{
    if (m_mapped) {
        qCCritical(RENDER_INVALID_OPERATION, "Attempting to Bind a locked buffer");
        Q_ASSERT(false);
    }

    m_backend->bindBuffer(m_handle, m_type);
}

void QSSGRenderConstantBuffer::bindToShaderProgram(const QSSGRef<QSSGRenderShaderProgram> &inShader, quint32 blockIndex, quint32 binding)
{
    if ((qint32)binding == -1) {
        binding = m_context->nextConstantBufferUnit();
        m_backend->programSetConstantBlock(inShader->handle(), blockIndex, binding);
    }

    m_backend->programSetConstantBuffer(binding, m_handle);
}

bool QSSGRenderConstantBuffer::setupBuffer(const QSSGRenderShaderProgram *program, qint32 index, qint32 bufSize, qint32 paramCount)
{
    bool bSuccess = false;

    if (!m_hwBufferInitialized) {
        // allocate shadow buffer
        QByteArray newShadowCopy;
        newShadowCopy.resize(bufSize);
        quint8 *newMem = reinterpret_cast<quint8 *>(newShadowCopy.data());

        // allocate temp buffers to hold constant buffer information
        qint32 *theIndices = nullptr;
        QSSGRenderShaderDataType *theTypes = nullptr;
        qint32 *theSizes = nullptr;
        qint32 *theOffsets = nullptr;

        theIndices = static_cast<qint32 *>(::malloc(size_t(paramCount) * sizeof(qint32)));
        if (!theIndices)
            goto fail;
        theTypes = static_cast<QSSGRenderShaderDataType *>(::malloc(size_t(paramCount) * sizeof(QSSGRenderShaderDataType)));
        if (!theTypes)
            goto fail;
        theSizes = static_cast<qint32 *>(::malloc(size_t(paramCount) * sizeof(qint32)));
        if (!theSizes)
            goto fail;
        theOffsets = static_cast<qint32 *>(::malloc(size_t(paramCount) * sizeof(qint32)));
        if (!theOffsets)
            goto fail;

        bSuccess = true;

        // get indices for the individal constant buffer entries
        m_backend->getConstantBufferParamIndices(program->handle(), index, theIndices);

        // get constant buffer uniform information
        m_backend->getConstantBufferParamInfoByIndices(program->handle(), paramCount, (quint32 *)theIndices, theTypes, theSizes, theOffsets);

        // get the names of the uniforms
        char nameBuf[512];
        qint32 elementCount, binding;
        QSSGRenderShaderDataType type;

        for (int idx = 0; idx != paramCount; ++idx) {
            m_backend->getConstantInfoByID(program->handle(), theIndices[idx], 512, &elementCount, &type, &binding, nameBuf);
            // check if we already have this entry
            const QByteArray theName = nameBuf;
            ParamHandle h = ParamHandle::create(theName);
            auto entry = m_constantBufferEntryMap.constFind(h);
            if (entry != m_constantBufferEntryMap.cend()) {
                ConstantBufferParamEntry *pParam = entry.value();
                // copy content
                if (m_shadowCopy.size())
                    memcpy(newMem + theOffsets[idx],
                           m_shadowCopy.constData() + entry.value()->m_offset,
                           entry.value()->m_count * uniformTypeSize(pParam->m_type));

                pParam->m_offset = theOffsets[idx];
                Q_ASSERT(type == pParam->m_type);
                Q_ASSERT(elementCount == pParam->m_count);
            } else {
                // create one
                m_constantBufferEntryMap.insert(h,
                                                createParamEntry(theName,
                                                                 theTypes[idx],
                                                                 theSizes[idx],
                                                                 theOffsets[idx]));
            }
        }

        m_shadowCopy = newShadowCopy;
        m_hwBufferInitialized = true;

    fail:
        if (theIndices)
            ::free(theIndices);
        if (theTypes)
            ::free(theTypes);
        if (theSizes)
            ::free(theSizes);
        if (theOffsets)
            ::free(theOffsets);

    } else {
        // some sanity checks
        bSuccess = true;
        bSuccess &= (m_shadowCopy.size() <= bufSize);
    }

    return bSuccess;
}

void QSSGRenderConstantBuffer::update()
{
    // we only update the buffer if the buffer is already on hardware
    // and if it is dirty
    if (m_hwBufferInitialized && (m_rangeStart < m_rangeEnd)) {
        if (m_rangeStart == 0 && m_rangeEnd >= quint32(m_shadowCopy.size())) {
            m_backend->updateBuffer(m_handle, m_type, m_usageType, toByteView(m_shadowCopy));
        } else {
            Q_ASSERT(m_rangeStart < m_rangeEnd && m_rangeEnd <= quint32(m_shadowCopy.size()));
            m_backend->updateBufferRange(m_handle,
                                         m_type,
                                         m_rangeStart,
                                         QSSGByteView(m_shadowCopy.constBegin() + m_rangeStart, m_rangeEnd - m_rangeStart));
        }

        m_rangeStart = std::numeric_limits<quint32>::max();
        m_rangeEnd = 0;
    }
}

void QSSGRenderConstantBuffer::addParam(const ParamHandle &handle, QSSGRenderShaderDataType type, qint32 count)
{
    const auto it = m_constantBufferEntryMap.constFind(handle);
    const auto end = m_constantBufferEntryMap.cend();
    if (it != end) // no duplicated entries
        return;

    ConstantBufferParamEntry *newEntry = new ConstantBufferParamEntry(handle.name, type, count, m_currentOffset);
    m_constantBufferEntryMap.insert(handle, newEntry);

    // compute new current buffer size and offset
    qint32 constantSize = uniformTypeSize(type) * count;
    m_currentSize += constantSize;
    m_currentOffset += constantSize;
}

void QSSGRenderConstantBuffer::updateParam(const ParamHandle &handle, QSSGByteView value)
{
    // allocate space if not done yet
    // NOTE this gets reallocated once we get the real constant buffer size from a program
    if (!m_shadowCopy.size())
        m_shadowCopy.resize(m_currentSize);

    const auto entry = m_constantBufferEntryMap.constFind(handle);
    if (entry != m_constantBufferEntryMap.cend()) {
        const qint32 size = entry.value()->m_count * uniformTypeSize(entry.value()->m_type);
        Q_ASSERT(size == value.size());
        if (!memcmp(m_shadowCopy.constBegin() + entry.value()->m_offset, value.begin(), size_t(size)))
            return;
        memcpy(m_shadowCopy.begin() + entry.value()->m_offset, value.begin(), size_t(size));
        setDirty(entry.value()->m_offset, size);
    }
}

void QSSGRenderConstantBuffer::updateRaw(quint32 offset, QSSGByteView data)
{
    // allocate space if yet done
    if (!m_shadowCopy.size()) {
        Q_ASSERT(offset == 0);
        m_shadowCopy.resize(data.size());
    }

    Q_ASSERT((offset + data.size()) < (quint32)m_maxBlockSize);

    // we do not initialize anything when this is used
    m_hwBufferInitialized = true;

    // we do not allow resize once allocated
    if ((offset + data.size()) > quint32(m_shadowCopy.size()))
        return;

    // copy data
    if (!memcmp(m_shadowCopy.constBegin() + offset, data.begin(), data.size())) {
        return;
    }
    memcpy(m_shadowCopy.begin() + offset, data.begin(), data.size());

    setDirty(offset, data.size());
}

ConstantBufferParamEntry *QSSGRenderConstantBuffer::createParamEntry(const QByteArray &name,
                                                                       QSSGRenderShaderDataType type,
                                                                       qint32 count,
                                                                       qint32 offset)
{
    ConstantBufferParamEntry *newEntry = new ConstantBufferParamEntry(name, type, count, offset);

    return newEntry;
}

qint32 QSSGRenderConstantBuffer::uniformTypeSize(QSSGRenderShaderDataType type)
{
    switch (type) {
    case QSSGRenderShaderDataType::Float:
        return sizeof(float);
    case QSSGRenderShaderDataType::Integer:
        return sizeof(qint32);
    case QSSGRenderShaderDataType::IntegerVec2:
        return sizeof(qint32) * 2;
    case QSSGRenderShaderDataType::IntegerVec3:
        return sizeof(qint32) * 3;
    case QSSGRenderShaderDataType::IntegerVec4:
        return sizeof(qint32) * 4;
    case QSSGRenderShaderDataType::UnsignedInteger:
        return sizeof(quint32);
    case QSSGRenderShaderDataType::UnsignedIntegerVec2:
        return sizeof(quint32) * 2;
    case QSSGRenderShaderDataType::UnsignedIntegerVec3:
        return sizeof(quint32) * 3;
    case QSSGRenderShaderDataType::UnsignedIntegerVec4:
        return sizeof(quint32) * 4;
    case QSSGRenderShaderDataType::Vec2:
        return sizeof(float) * 2;
    case QSSGRenderShaderDataType::Vec3:
        return sizeof(float) * 3;
    case QSSGRenderShaderDataType::Vec4:
        return sizeof(float) * 4;
    case QSSGRenderShaderDataType::Matrix3x3:
        return sizeof(float) * 9;
    case QSSGRenderShaderDataType::Matrix4x4:
        return sizeof(float) * 16;
    default:
        Q_ASSERT_X(0, "Unhandled type",  "QSSGRenderConstantBuffer::getUniformTypeSize");
        break;
    }

    return 0;
}

QT_END_NAMESPACE
