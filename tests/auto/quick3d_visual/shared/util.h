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

#ifndef QUICK3D_VISUAL_TEST_UTIL_H
#define QUICK3D_VISUAL_TEST_UTIL_H

#include <QtCore/QDir>
#include <QtCore/QUrl>
#include <QtCore/QStringList>
#include <QtGui/QImage>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuick/QQuickView>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickRenderControl>
#include <QtQuick/QQuickRenderTarget>
#include <QtQuick/QQuickGraphicsDevice>
#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

#include <QtGui/private/qrhi_p.h>
#include <QtQuick/private/qquickrendercontrol_p.h>

/* Base class for tests with data that are located in a "data" subfolder. */

class QQuick3DDataTest : public QObject
{
    Q_OBJECT
public:
    QQuick3DDataTest();
    ~QQuick3DDataTest();

    bool initialized() const { return m_initialized; }

    static bool isRunningOnRhi();
    QQuickView *createView(const QString &filename, const QSize &windowSize);
    QImage grab(QQuickWindow *window);

    bool comparePixel(const QImage &image, int logicalX, int logicalY, qreal dpr, const QColor &expected, int fuzz = 2);
    inline bool comparePixel(const QImage &image, const QPoint &logicalPos, qreal dpr, const QColor &expected, int fuzz = 2)
    {
        return comparePixel(image, logicalPos.x(), logicalPos.y(), dpr, expected, fuzz);
    }
    bool comparePixelNormPos(const QImage &image, qreal normalizedX, qreal normalizedY, const QColor &expected, int fuzz = 2);

    QString testFile(const QString &fileName) const;
    inline QString testFile(const char *fileName) const
        { return testFile(QLatin1String(fileName)); }
    inline QUrl testFileUrl(const QString &fileName) const
        {
            const QString fn = testFile(fileName);
            return fn.startsWith(QLatin1Char(':'))
                ? QUrl(QLatin1String("qrc") + fn)
                : QUrl::fromLocalFile(fn);
        }
    inline QUrl testFileUrl(const char *fileName) const
        { return testFileUrl(QLatin1String(fileName)); }

    inline QString dataDirectory() const { return m_dataDirectory; }
    inline QUrl dataDirectoryUrl() const { return m_dataDirectoryUrl; }
    inline QString directory() const  { return m_directory; }

    static inline QQuick3DDataTest *instance() { return m_instance; }

public slots:
    virtual void initTestCase();
    virtual void cleanupTestCase();

private:
    static QQuick3DDataTest *m_instance;

    bool m_initialized;
    QString m_dataDirectory;
    QUrl m_dataDirectoryUrl;
    QString m_directory;
};

struct QQuick3DTestOffscreenRenderer
{
public:
    bool init(const QUrl &fileUrl, void *vulkanInstance);
    QRhiCommandBuffer *activeCommandBuffer() {
        QQuickRenderControlPrivate *rd = QQuickRenderControlPrivate::get(renderControl.data());
        return rd->cb;
    }
    void enqueueReadback(bool *readCompleted, QRhiReadbackResult *readResult, QImage *result);
    bool resize(const QSize &newSize);

    QScopedPointer<QQuickRenderControl> renderControl;
    QScopedPointer<QQuickWindow> quickWindow;
    QScopedPointer<QQmlEngine> qmlEngine;
    QScopedPointer<QQmlComponent> qmlComponent;
    QScopedPointer<QRhiTexture> tex;
    QScopedPointer<QRhiRenderBuffer> ds;
    QScopedPointer<QRhiTextureRenderTarget> texRt;
    QScopedPointer<QRhiRenderPassDescriptor> rp;
    QRhi *rhi = nullptr;
    QQuickItem *rootItem = nullptr;
};

class QQuick3DTestMessageHandler
{
    Q_DISABLE_COPY(QQuick3DTestMessageHandler)
public:
    QQuick3DTestMessageHandler();
    ~QQuick3DTestMessageHandler();

    const QStringList &messages() const { return m_messages; }
    const QString messageString() const { return m_messages.join(QLatin1Char('\n')); }

    void clear() { m_messages.clear(); }

    void setIncludeCategoriesEnabled(bool enabled) { m_includeCategories = enabled; }

private:
    static void messageHandler(QtMsgType, const QMessageLogContext &context, const QString &message);

    static QQuick3DTestMessageHandler *m_instance;
    QStringList m_messages;
    QtMessageHandler m_oldHandler;
    bool m_includeCategories;
};

#endif
