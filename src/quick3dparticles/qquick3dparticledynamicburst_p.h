/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#ifndef QQUICK3DPARTICLEDYNAMICBURST_H
#define QQUICK3DPARTICLEDYNAMICBURST_H

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

#include <QtQuick3DParticles/private/qquick3dparticleemitburst_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DPARTICLES_EXPORT QQuick3DParticleDynamicBurst : public QQuick3DParticleEmitBurst
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int amountVariation READ amountVariation WRITE setAmountVariation NOTIFY amountVariationChanged)
    Q_PROPERTY(TriggerMode triggerMode READ triggerMode WRITE setTriggerMode NOTIFY triggerModeChanged)
    QML_NAMED_ELEMENT(DynamicBurst3D)
    QML_ADDED_IN_VERSION(6, 3)

public:
    enum TriggerMode
    {
        TriggerTime = 0,
        TriggerStart,
        TriggerEnd
    };
    Q_ENUM(TriggerMode)

    explicit QQuick3DParticleDynamicBurst(QObject *parent = nullptr);
    bool enabled() const;
    int amountVariation() const;
    TriggerMode triggerMode() const;

public Q_SLOTS:
    void setEnabled(bool enabled);
    void setAmountVariation(int value);
    void setTriggerMode(TriggerMode mode);

Q_SIGNALS:
    void enabledChanged();
    void amountVariationChanged();
    void triggerModeChanged();

private:
    friend class QQuick3DParticleEmitter;

    bool m_enabled = true;
    int m_amountVariation = 0;
    TriggerMode m_triggerMode = TriggerMode::TriggerTime;
};

QT_END_NAMESPACE

#endif // QQUICK3DPARTICLEDYNAMICBURST_H
