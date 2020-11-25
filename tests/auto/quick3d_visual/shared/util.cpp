/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "util.h"

#include <QtCore/QDebug>
#include <QtCore/QMutexLocker>

#include <QtQuick3D/qquick3d.h>

QQuick3DDataTest *QQuick3DDataTest::m_instance = 0;

QQuick3DDataTest::QQuick3DDataTest() :
    m_initialized(false),
#ifdef QT_TESTCASE_BUILDDIR
    m_dataDirectory(QTest::qFindTestData("data", QT_QMLTEST_DATADIR, 0, QT_TESTCASE_BUILDDIR)),
#else
    m_dataDirectory(QTest::qFindTestData("data", QT_QMLTEST_DATADIR, 0)),
#endif

    m_dataDirectoryUrl(m_dataDirectory.startsWith(QLatin1Char(':'))
        ? QUrl(QLatin1String("qrc") + m_dataDirectory)
        : QUrl::fromLocalFile(m_dataDirectory + QLatin1Char('/')))
{
    m_instance = this;
}

QQuick3DDataTest::~QQuick3DDataTest()
{
    m_instance = 0;
}

void QQuick3DDataTest::initTestCase()
{
    QVERIFY2(!m_dataDirectory.isEmpty(), "'data' directory not found");
    m_directory = QFileInfo(m_dataDirectory).absolutePath();
    if (m_dataDirectoryUrl.scheme() != QLatin1String("qrc"))
        QVERIFY2(QDir::setCurrent(m_directory), qPrintable(QLatin1String("Could not chdir to ") + m_directory));

    if (QGuiApplication::platformName() == QLatin1String("offscreen")
        || QGuiApplication::platformName() == QLatin1String("minimal"))
    {
        QSKIP("Skipping visual 3D tests due to running with offscreen/minimal");
    }

    if (!isRunningOnRhi())
        QSKIP("Skipping visual 3D tests due to not running with QRhi");

#if QT_CONFIG(opengl)
    QSurfaceFormat::setDefaultFormat(QQuick3D::idealSurfaceFormat());
#endif

    m_initialized = true;
}

void QQuick3DDataTest::cleanupTestCase()
{
    m_initialized = false;
}

QString QQuick3DDataTest::testFile(const QString &fileName) const
{
    if (m_directory.isEmpty())
        qFatal("QQuick3DDataTest::initTestCase() not called.");
    QString result = m_dataDirectory;
    result += QLatin1Char('/');
    result += fileName;
    return result;
}

bool QQuick3DDataTest::isRunningOnRhi()
{
    static bool retval = false;
    static bool decided = false;
    if (!decided) {
        decided = true;
        QQuickView dummy;
        dummy.show();
        if (!QTest::qWaitForWindowExposed(&dummy)) {
            [](){ QFAIL("Could not show a QQuickView"); }();
            return false;
        }
        QSGRendererInterface::GraphicsApi api = dummy.rendererInterface()->graphicsApi();
        retval = QSGRendererInterface::isApiRhiBased(api);
        dummy.hide();
    }
    return retval;
}

QQuickView *QQuick3DDataTest::createView(const QString &filename, const QSize &windowSize)
{
    QQuickView *view = new QQuickView;
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->resize(windowSize);
    view->setSource(testFileUrl(filename));
    view->show();
    return view;
}

QImage QQuick3DDataTest::grab(QQuickWindow *window)
{
    const qreal dpr = window->devicePixelRatio();
    QImage content = window->grabWindow();
    if (content.isNull()) {
        [](){ QFAIL("No QImage received from grabWindow"); }();
        return QImage();
    }
    const QSize expectedSize = window->size() * dpr;
    if (content.size() != expectedSize) {
        qDebug() << "grabbed image size" << content.size() << "expected size" << expectedSize;
        [](){ QFAIL("Unexpected QImage size from grabWindow"); }();
        return QImage();
    }
    if (qEnvironmentVariableIntValue("QT_QUICK3D_TEST_DEBUG")) {
        static int cnt = 1;
        const QString fn = QString::asprintf("quick3d_test_%d.png", cnt);
        ++cnt;
        content.save(fn);
        qDebug("grab result saved to %s", qPrintable(fn));
    }
    return content;
}

static inline bool compareColor(int a, int b, int fuzz, int debugX, int debugY, const char *debugChannelName)
{
    const int diff = qAbs(a - b);
    if (diff > fuzz) {
        qDebug("%s channel mismatch at position (%d, %d). Expected %d with maximum difference %d, got %d.",
               debugChannelName, debugX, debugY, a, fuzz, b);
        return false;
    }
    return true;
}

static inline bool compareColorAt(const QImage &image, int physicalX, int physicalY, const QColor &expected, int fuzz)
{
    const QRgb c = image.pixel(physicalX, physicalY);
    if (!compareColor(qRed(c), expected.red(), fuzz, physicalX, physicalY, "Red"))
        return false;
    if (!compareColor(qGreen(c), expected.green(), fuzz, physicalX, physicalY, "Green"))
        return false;
    if (!compareColor(qBlue(c), expected.blue(), fuzz, physicalX, physicalY, "Blue"))
        return false;
    if (!compareColor(qAlpha(c), expected.alpha(), fuzz, physicalX, physicalY, "Alpha"))
        return false;
    return true;
}

bool QQuick3DDataTest::comparePixel(const QImage &image, int logicalX, int logicalY, qreal dpr, const QColor &expected, int fuzz)
{
    const int physicalX = qCeil(logicalX * dpr);
    const int physicalY = qCeil(logicalY * dpr);
    return compareColorAt(image, physicalX, physicalY, expected, fuzz);
}

bool QQuick3DDataTest::comparePixelNormPos(const QImage &image, qreal normalizedX, qreal normalizedY, const QColor &expected, int fuzz)
{
    const int physicalWidth = image.width();
    const int physicalHeight = image.height();
    const int physicalX = qCeil(physicalWidth * normalizedX);
    const int physicalY = qCeil(physicalHeight * normalizedY);
    return compareColorAt(image, physicalX, physicalY, expected, fuzz);
}

Q_GLOBAL_STATIC(QMutex, qQmlTestMessageHandlerMutex)

QQuick3DTestMessageHandler *QQuick3DTestMessageHandler::m_instance = 0;

void QQuick3DTestMessageHandler::messageHandler(QtMsgType, const QMessageLogContext &context, const QString &message)
{
    QMutexLocker locker(qQmlTestMessageHandlerMutex());
    if (QQuick3DTestMessageHandler::m_instance) {
        if (QQuick3DTestMessageHandler::m_instance->m_includeCategories)
            QQuick3DTestMessageHandler::m_instance->m_messages.push_back(QString("%1: %2").arg(context.category, message));
        else
            QQuick3DTestMessageHandler::m_instance->m_messages.push_back(message);
    }
}

QQuick3DTestMessageHandler::QQuick3DTestMessageHandler()
{
    QMutexLocker locker(qQmlTestMessageHandlerMutex());
    Q_ASSERT(!QQuick3DTestMessageHandler::m_instance);
    QQuick3DTestMessageHandler::m_instance = this;
    m_oldHandler = qInstallMessageHandler(messageHandler);
    m_includeCategories = false;
}

QQuick3DTestMessageHandler::~QQuick3DTestMessageHandler()
{
    QMutexLocker locker(qQmlTestMessageHandlerMutex());
    Q_ASSERT(QQuick3DTestMessageHandler::m_instance);
    qInstallMessageHandler(m_oldHandler);
    QQuick3DTestMessageHandler::m_instance = 0;
}
