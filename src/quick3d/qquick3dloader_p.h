/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QQUICK3DLOADER_P_H
#define QQUICK3DLOADER_P_H

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
#include <QQmlIncubator>
#include <private/qqmlguard_p.h>

#include <private/qv4value_p.h>

QT_BEGIN_NAMESPACE
class QQuick3DLoader;
class QQuick3DLoaderIncubator : public QQmlIncubator
{
public:
    QQuick3DLoaderIncubator(QQuick3DLoader *l, IncubationMode mode)
        : QQmlIncubator(mode), m_loader(l) {}

protected:
    void statusChanged(Status) override;
    void setInitialState(QObject *) override;

private:
    QQuick3DLoader *m_loader;
};

class QQmlContext;

class Q_QUICK3D_EXPORT QQuick3DLoader : public QQuick3DNode
{
    Q_OBJECT

    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QQmlComponent *sourceComponent READ sourceComponent WRITE setSourceComponent RESET resetSourceComponent NOTIFY sourceComponentChanged)
    Q_PROPERTY(QObject *item READ item NOTIFY itemChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool asynchronous READ asynchronous WRITE setAsynchronous NOTIFY asynchronousChanged)

    QML_NAMED_ELEMENT(Loader3D)

public:
    explicit QQuick3DLoader(QQuick3DNode *parent = nullptr);
    ~QQuick3DLoader() override;

    bool active() const;
    void setActive(bool newVal);

    Q_INVOKABLE void setSource(QQmlV4Function *);

    QUrl source() const;
    void setSource(const QUrl &);

    QQmlComponent *sourceComponent() const;
    void setSourceComponent(QQmlComponent *);
    void resetSourceComponent();

    enum Status { Null, Ready, Loading, Error };
    Q_ENUM(Status)
    Status status() const;
    qreal progress() const;

    bool asynchronous() const;
    void setAsynchronous(bool a);

    QObject *item() const;

Q_SIGNALS:
    void itemChanged();
    void activeChanged();
    void sourceChanged();
    void sourceComponentChanged();
    void statusChanged();
    void progressChanged();
    void loaded();
    void asynchronousChanged();

protected:
    void componentComplete() override;

private Q_SLOTS:
    void sourceLoaded();

private:
    Q_DISABLE_COPY(QQuick3DLoader)
    friend QQuick3DLoaderIncubator;
    void setSource(const QUrl &sourceUrl, bool needsClear);
    void loadFromSource();
    void loadFromSourceComponent();
    void clear();
    void load();

    void incubatorStateChanged(QQmlIncubator::Status status);
    void setInitialState(QObject *obj);
    void disposeInitialPropertyValues();
    static QUrl resolveSourceUrl(QQmlV4Function *args);
    QV4::ReturnedValue extractInitialPropertyValues(QQmlV4Function *args, bool *error);

    void createComponent();

    QUrl m_source;
    QQuick3DNode *m_item;
    QObject *m_object;
    QQmlStrongJSQObjectReference<QQmlComponent> m_component;
    QQmlContext *m_itemContext;
    QQuick3DLoaderIncubator *m_incubator;
    QV4::PersistentValue m_initialPropertyValues;
    QV4::PersistentValue m_qmlCallingContext;
    bool m_active : 1;
    bool m_loadingFromSource : 1;
    bool m_asynchronous : 1;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuick3DLoader)

#endif // QQUICK3DLOADER_P_H
