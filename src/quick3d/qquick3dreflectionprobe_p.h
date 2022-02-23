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

#ifndef QSSGREFLECTIONPROBE_H
#define QSSGREFLECTIONPROBE_H

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

#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QColor>

QT_BEGIN_NAMESPACE

class Q_QUICK3D_EXPORT QQuick3DReflectionProbe : public QQuick3DNode
{
    Q_OBJECT
    Q_PROPERTY(ReflectionQuality quality READ quality WRITE setQuality NOTIFY qualityChanged)
    Q_PROPERTY(QColor clearColor READ clearColor WRITE setClearColor NOTIFY clearColorChanged)
    Q_PROPERTY(ReflectionRefreshMode refreshMode READ refreshMode WRITE setRefreshMode NOTIFY refreshModeChanged)
    Q_PROPERTY(ReflectionTimeSlicing timeSlicing READ timeSlicing WRITE setTimeSlicing NOTIFY timeSlicingChanged)
    Q_PROPERTY(bool parallaxCorrection READ parallaxCorrection WRITE setParallaxCorrection NOTIFY parallaxCorrectionChanged)
    Q_PROPERTY(QVector3D boxSize READ boxSize WRITE setBoxSize NOTIFY boxSizeChanged)
    QML_NAMED_ELEMENT(ReflectionProbe)
    QML_ADDED_IN_VERSION(6, 3)

public:
    enum class ReflectionQuality {
        VeryLow,
        Low,
        Medium,
        High,
        VeryHigh
    };
    Q_ENUM(ReflectionQuality)

    enum class ReflectionRefreshMode {
        FirstFrame,
        EveryFrame
    };
    Q_ENUM(ReflectionRefreshMode)

    enum class ReflectionTimeSlicing {
        None,
        AllFacesAtOnce,
        IndividualFaces,
    };
    Q_ENUM(ReflectionTimeSlicing)

    explicit QQuick3DReflectionProbe(QQuick3DNode *parent = nullptr);
    ~QQuick3DReflectionProbe() override { }

    ReflectionQuality quality() const;
    QColor clearColor() const;
    ReflectionRefreshMode refreshMode() const;
    ReflectionTimeSlicing timeSlicing() const;
    bool parallaxCorrection() const;
    QVector3D boxSize() const;

public Q_SLOTS:
    void setQuality(ReflectionQuality reflectionQuality);
    void setClearColor(const QColor &clearColor);
    void setRefreshMode(ReflectionRefreshMode newRefreshMode);
    void setTimeSlicing(ReflectionTimeSlicing newTimeSlicing);
    void setParallaxCorrection(bool parallaxCorrection);
    void setBoxSize(const QVector3D &newBoxSize);

Q_SIGNALS:
    void qualityChanged();
    void clearColorChanged();
    void refreshModeChanged();
    void timeSlicingChanged();
    void parallaxCorrectionChanged();

    void boxSizeChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void markAllDirty() override;

    enum class DirtyFlag {
        QualityDirty = (1 << 0),
        ClearColorDirty = (1 << 1),
        RefreshModeDirty = (1 << 2),
        ParallaxCorrectionDirty = (1 << 3),
        BoxDirty = (1 << 4),
        TimeSlicingDirty = (1 << 5)
    };
    Q_DECLARE_FLAGS(DirtyFlags, DirtyFlag)

    DirtyFlags m_dirtyFlags = DirtyFlags(DirtyFlag::QualityDirty)
                              | DirtyFlags(DirtyFlag::ClearColorDirty)
                              | DirtyFlags(DirtyFlag::RefreshModeDirty)
                              | DirtyFlags(DirtyFlag::ParallaxCorrectionDirty)
                              | DirtyFlags(DirtyFlag::BoxDirty)
                              | DirtyFlags(DirtyFlag::TimeSlicingDirty);

private:
    quint32 mapToReflectionResolution(ReflectionQuality quality);
    ReflectionQuality m_quality = ReflectionQuality::Low;
    QColor m_clearColor = Qt::transparent;
    ReflectionRefreshMode m_refreshMode = ReflectionRefreshMode::EveryFrame;
    bool m_parallaxCorrection = false;
    QVector3D m_boxSize = QVector3D(0, 0, 0);
    ReflectionTimeSlicing m_timeSlicing = ReflectionTimeSlicing::None;
};

QT_END_NAMESPACE

#endif // QSSGREFLECTIONPROBE_H
