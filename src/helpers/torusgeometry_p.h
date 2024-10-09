// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TORUSGEOMETRY_P_H
#define TORUSGEOMETRY_P_H

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

#include <QQuick3DGeometry>
#include <QQmlEngine>
#include <QVector3D>

#if QT_CONFIG(concurrent)
#include <QFuture>
#include <QFutureWatcher>
#endif

QT_BEGIN_NAMESPACE

class TorusGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    Q_PROPERTY(int rings READ rings WRITE setRings NOTIFY ringsChanged FINAL)
    Q_PROPERTY(int segments READ segments WRITE setSegments NOTIFY segmentsChanged FINAL)
    Q_PROPERTY(float radius READ radius WRITE setRadius NOTIFY radiusChanged FINAL)
    Q_PROPERTY(float tubeRadius READ tubeRadius WRITE setTubeRadius NOTIFY tubeRadiusChanged FINAL)
    Q_PROPERTY(bool asynchronous READ asynchronous WRITE setAsynchronous NOTIFY asynchronousChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    QML_ELEMENT
public:
    enum Status { Null, Ready, Loading, Error };
    Q_ENUM(Status)
    explicit TorusGeometry(QQuick3DObject *parent = nullptr);
    ~TorusGeometry() override;

    int rings() const;
    void setRings(int newRings);
    int segments() const;
    void setSegments(int newSegments);

    float radius() const;
    void setRadius(float newRadius);

    float tubeRadius() const;
    void setTubeRadius(float newTubeRadius);

    bool asynchronous() const;
    void setAsynchronous(bool newAsynchronous);

    Status status() const;

private Q_SLOTS:
    void doUpdateGeometry();
    void requestFinished();

Q_SIGNALS:
    void ringsChanged();
    void segmentsChanged();
    void radiusChanged();
    void tubeRadiusChanged();
    void asynchronousChanged();
    void statusChanged();

private:
    struct GeometryData {
        QByteArray vertexData;
        QByteArray indexData;
        QVector3D boundsMin;
        QVector3D boundsMax;
    };

    void scheduleGeometryUpdate();
    void updateGeometry(const GeometryData &geometryData);

    static TorusGeometry::GeometryData generateTorusGeometry(int rings,
                                                             int segments,
                                                             float radius,
                                                             float tubeRadius);
#if QT_CONFIG(concurrent)
    static void generateTorusGeometryAsync(QPromise<TorusGeometry::GeometryData> &promise,
                                           int rings,
                                           int segments,
                                           float radius,
                                           float tubeRadius);
#endif

    int m_rings = 50;
    int m_segments = 50;
    float m_radius = 100.0f;
    float m_tubeRadius = 10.0f;
    bool m_asynchronous = true;
    Status m_status = Null;
#if QT_CONFIG(concurrent)
    QFuture<GeometryData> m_geometryDataFuture;
    QFutureWatcher<GeometryData> m_geometryDataWatcher;
#endif
    bool m_geometryUpdateRequested = false;
    bool m_pendingAsyncUpdate = false;
};

QT_END_NAMESPACE

#endif // TORUSGEOMETRY_P_H
