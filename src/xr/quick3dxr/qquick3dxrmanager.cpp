// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dxrmanager_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

#include <rhi/qrhi.h>

#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/QQuickRenderControl>
#include <QtQuick/QQuickRenderTarget>
#include <QtQuick/QQuickItem>

#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>


// #include "qquick3dxrcamera_p.h"
#include "qquick3dxranimationdriver_p.h"

#if defined(Q_OS_VISIONOS)
# include <QtQuick3DXr/private/qquick3dxrmanager_visionos_p.h>
#elif defined(USE_OPENXR)
# include "openxr/qquick3dxrmanager_openxr_p.h"
#endif

#include "qquick3dxrorigin_p.h"

QT_BEGIN_NAMESPACE


QQuick3DXrManager::QQuick3DXrManager(QObject *parent)
    : QObject(parent)
    , d_ptr(new QQuick3DXrManagerPrivate(*this))
{
}

QQuick3DXrManager::~QQuick3DXrManager()
{
    teardown();

    // maintain the correct order
    delete m_vrViewport;
    delete m_quickWindow;
    delete m_renderControl;
    delete m_animationDriver;
}

bool QQuick3DXrManager::isReady() const
{
    Q_D(const QQuick3DXrManager);
    return d->isReady();
}

bool QQuick3DXrManager::initialize()
{
    Q_D(QQuick3DXrManager);

    QString m_errorString;

    // TODO: Handle visionos being async a bit better
    if (!d->initialize()) {
        if (!d->isReady())
            m_errorString = QStringLiteral("Waiting for the renderer to start.");
        else
            m_errorString = QStringLiteral("Failed to initialize the XR manager.");

        return false;
    }

    // Setup Graphics
    return setupGraphics();
}

void QQuick3DXrManager::teardown()
{
    Q_D(QQuick3DXrManager);
    d->teardown();
}

bool QQuick3DXrManager::isValid() const
{
    Q_D(const QQuick3DXrManager);
    return d->isValid();
}

void QQuick3DXrManager::setPassthroughEnabled(bool enabled)
{
    Q_D(QQuick3DXrManager);
    d->setPassthroughEnabled(enabled);
}

bool QQuick3DXrManager::isPassthroughEnabled() const
{
    Q_D(const QQuick3DXrManager);
    return d->isPassthroughEnabled();
}

void QQuick3DXrManager::setMultiviewRenderingEnabled(bool enable)
{
    Q_D(QQuick3DXrManager);
    d->setMultiviewRenderingEnabled(enable);
}

bool QQuick3DXrManager::isMultiViewRenderingEnabled() const
{
    Q_D(const QQuick3DXrManager);
    return d->isMultiViewRenderingEnabled();
}

bool QQuick3DXrManager::isMultiViewRenderingSupported() const
{
    Q_D(const QQuick3DXrManager);
    return d->isMultiViewRenderingSupported();
}

QtQuick3DXr::FoveationLevel QQuick3DXrManager::getFixedFoveationLevel() const
{
#if defined(Q_OS_VISIONOS)
    // Foveation is not configurable on VisionOS
    return QtQuick3DXr::FoveationLevel::HighFoveation;
#else
    Q_D(const QQuick3DXrManager);
    return QtQuick3DXr::FoveationLevel(d->m_foveationLevel);
#endif
}

void QQuick3DXrManager::setFixedFoveationLevel(QtQuick3DXr::FoveationLevel level)
{
#if defined(Q_OS_VISIONOS)
    // Foveation is not configurable on VisionOS
    Q_UNUSED(level);
#else
    Q_D(QQuick3DXrManager);
    const XrFoveationLevelFB xrLevel = XrFoveationLevelFB(level);
    if (d->m_foveationLevel == xrLevel)
        return;

    d->m_foveationLevel = xrLevel;
    d->setupMetaQuestFoveation();
#endif
}

QtQuick3DXr::ReferenceSpace QQuick3DXrManager::getReferenceSpace() const
{
    Q_D(const QQuick3DXrManager);
    return d->getReferenceSpace();
}

void QQuick3DXrManager::setReferenceSpace(QtQuick3DXr::ReferenceSpace newReferenceSpace)
{
    Q_D(QQuick3DXrManager);

    d->setReferenceSpace(newReferenceSpace);
}

bool QQuick3DXrManager::isDepthSubmissionEnabled() const
{
    Q_D(const QQuick3DXrManager);
    return d->isDepthSubmissionEnabled();
}

void QQuick3DXrManager::setDepthSubmissionEnabled(bool enable)
{
    Q_D(QQuick3DXrManager);
    d->setDepthSubmissionEnabled(enable);
}

QString QQuick3DXrManager::errorString() const
{
    Q_D(const QQuick3DXrManager);
    return d->errorString();
}

void QQuick3DXrManager::setSamples(int samples)
{
    Q_D(QQuick3DXrManager);
    d->setSamples(samples);
}

void QQuick3DXrManager::update()
{
    QEvent *request = new QEvent(QEvent::UpdateRequest);
    QCoreApplication::postEvent(this, request);
}

bool QQuick3DXrManager::event(QEvent *e)
{
    Q_D(QQuick3DXrManager);

    if (e->type() == QEvent::UpdateRequest) {
        d->processXrEvents();
        update();
        return true;
    }
    return QObject::event(e);
}

QQuick3DXrInputManager *QQuick3DXrManager::getInputManager() const
{
#if !defined(Q_OS_VISIONOS)
    Q_D(const QQuick3DXrManager);
    return d->m_inputManager;
#else
    return nullptr;
#endif
}

bool QQuick3DXrManager::setupGraphics()
{
    Q_D(QQuick3DXrManager);

    // FIXME: Should probably make sure we don't accidentally get here more then once
    // or if we're re-initializing, in which case: make sure to clean up properly first.
    if (d->isGraphicsInitialized())
        return true;

    preSetupQuickScene();

    if (!d->setupGraphics(m_quickWindow))
        return false;

    if (!setupQuickScene())
        return false;

    QRhi *rhi = m_quickWindow->rhi();
    QSSG_ASSERT_X(rhi != nullptr, "No RHI handle!", return false);

    return d->finalizeGraphics(rhi);
}

void QQuick3DXrManager::renderFrame()
{
    Q_D(QQuick3DXrManager);

    checkOrigin();
    d->doRenderFrame();
}

void QQuick3DXrManager::preSetupQuickScene()
{
    if (!m_renderControl)
        m_renderControl = new QQuickRenderControl;
    if (!m_quickWindow)
        m_quickWindow = new QQuickWindow(m_renderControl);
}

bool QQuick3DXrManager::setupQuickScene()
{
    Q_D(QQuick3DXrManager);

    d->setupWindow(m_quickWindow);

    if (!m_animationDriver) {
        m_animationDriver = new QQuick3DXrAnimationDriver;
        m_animationDriver->install();
    }

    const bool initSuccess = m_renderControl->initialize();
    if (!initSuccess) {
        qWarning("Quick 3D XR: Failed to create renderControl (failed to initialize RHI?)");
        return false;
    }

    QRhi *rhi = m_renderControl->rhi();
    if (!rhi) {
        qWarning("Quick3D XR: No QRhi from renderControl. This should not happen.");
        return false;
    }

    qDebug("Quick 3D XR: QRhi initialized with backend %s", rhi->backendName());

    if (qEnvironmentVariableIntValue("QT_QUICK3D_XR_MULTIVIEW")) {
        qDebug("Quick3D XR: multiview rendering requested via the environment");
        setMultiviewRenderingEnabled(true);
    }

    return true;
}

void QQuick3DXrManager::checkOrigin()
{
    if (!m_xrOrigin) {
        // Check the scene for an XrOrigin
        std::function<QQuick3DXrOrigin*(QQuick3DObject *)> findOriginNode;
        findOriginNode = [&findOriginNode](QQuick3DObject *node) -> QQuick3DXrOrigin *{
            if (!node)
                return nullptr;
            auto origin = qobject_cast<QQuick3DXrOrigin *>(node);
            if (origin)
                return origin;
            for (auto child : node->childItems()) {
                origin = findOriginNode(child);
                if (origin)
                    return origin;
            }
            return nullptr;
        };
        auto origin = findOriginNode(m_vrViewport->importScene());
        if (origin) {
            m_xrOrigin = origin;
            emit xrOriginChanged();
            connect(m_xrOrigin, &QObject::destroyed, this, [this](){
                m_xrOrigin = nullptr;
               emit xrOriginChanged();
            });
        }
    }
}

bool QQuick3DXrManager::supportsPassthrough() const
{
    Q_D(const QQuick3DXrManager);
    return d->supportsPassthrough();
}

QT_END_NAMESPACE
