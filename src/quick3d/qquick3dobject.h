// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef Q_QUICK3D_OBJECT_H
#define Q_QUICK3D_OBJECT_H

#include <QtQuick3D/qtquick3dglobal.h>

#include <QtQml/qqml.h>
#include <QtQml/qqmlcomponent.h>

#include <QtCore/QObject>
#include <QtCore/qsharedpointer.h>

QT_BEGIN_NAMESPACE

class QQuick3DObjectPrivate;
class QQuick3DSceneManager;
class QSSGRenderGraphObject;

class Q_QUICK3D_EXPORT QQuick3DObject : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_DECLARE_PRIVATE(QQuick3DObject)
    Q_DISABLE_COPY(QQuick3DObject)

    Q_PROPERTY(QQuick3DObject *parent READ parentItem WRITE setParentItem NOTIFY parentChanged DESIGNABLE false FINAL)
    Q_PRIVATE_PROPERTY(QQuick3DObject::d_func(), QQmlListProperty<QObject> data READ data DESIGNABLE false)
    Q_PRIVATE_PROPERTY(QQuick3DObject::d_func(), QQmlListProperty<QObject> resources READ resources DESIGNABLE false)
    Q_PRIVATE_PROPERTY(QQuick3DObject::d_func(),
                       QQmlListProperty<QQuick3DObject> children READ children NOTIFY childrenChanged DESIGNABLE false)

    Q_PRIVATE_PROPERTY(QQuick3DObject::d_func(), QQmlListProperty<QQuickState> states READ states DESIGNABLE false)
    Q_PRIVATE_PROPERTY(QQuick3DObject::d_func(), QQmlListProperty<QQuickTransition> transitions READ transitions DESIGNABLE false)
    Q_PROPERTY(QString state READ state WRITE setState NOTIFY stateChanged)

    Q_CLASSINFO("DefaultProperty", "data")
    Q_CLASSINFO("qt_QmlJSWrapperFactoryMethod", "_q_createJSWrapper(QQmlV4ExecutionEnginePtr)")

    QML_NAMED_ELEMENT(Object3D)
    QML_UNCREATABLE("Object3D is Abstract")
public:
    enum ItemChange {
        ItemChildAddedChange, // value.item
        ItemChildRemovedChange, // value.item
        ItemSceneChange, // value.window
        ItemVisibleHasChanged, // value.boolValue
        ItemParentHasChanged, // value.item
        ItemOpacityHasChanged, // value.realValue
        ItemActiveFocusHasChanged, // value.boolValue
        ItemRotationHasChanged, // value.realValue
        ItemAntialiasingHasChanged, // value.boolValue
        ItemDevicePixelRatioHasChanged, // value.realValue
        ItemEnabledHasChanged // value.boolValue
    };

    union ItemChangeData {
        ItemChangeData(QQuick3DObject *v) : item(v) {}
        ItemChangeData(QQuick3DSceneManager *v) : sceneManager(v) {}
        ItemChangeData(qreal v) : realValue(v) {}
        ItemChangeData(bool v) : boolValue(v) {}

        QQuick3DObject *item;
        QQuick3DSceneManager *sceneManager;
        qreal realValue;
        bool boolValue;
    };

    explicit QQuick3DObject(QQuick3DObject *parent = nullptr);
    ~QQuick3DObject() override;

    QString state() const;
    void setState(const QString &state);

    QList<QQuick3DObject *> childItems() const;

    QQuick3DObject *parentItem() const;

public Q_SLOTS:
    void update();

    void setParentItem(QQuick3DObject *parentItem);

Q_SIGNALS:
    void parentChanged();
    void childrenChanged();
    void stateChanged();

protected:
    virtual QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node);
    virtual void markAllDirty();
    virtual void itemChange(ItemChange, const ItemChangeData &);
    explicit QQuick3DObject(QQuick3DObjectPrivate &dd, QQuick3DObject *parent = nullptr);

    void classBegin() override;
    void componentComplete() override;

    bool isComponentComplete() const;

    virtual void preSync();

private:
    Q_PRIVATE_SLOT(d_func(), void _q_resourceObjectDeleted(QObject *))
    Q_PRIVATE_SLOT(d_func(), quint64 _q_createJSWrapper(QQmlV4ExecutionEnginePtr))
    Q_PRIVATE_SLOT(d_func(), void _q_cleanupContentItem2D())

    friend class QQuick3DSceneManager;
};

class Q_QUICK3D_EXPORT QQuick3DContentLayer : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ContentLayer)
    QML_ADDED_IN_VERSION(6, 11)
    QML_SINGLETON
public:
    explicit QQuick3DContentLayer(QObject *parent = nullptr);
    ~QQuick3DContentLayer() override;

    enum LayerFlag : quint32 {
        LayerNone = 0x0,
        Layer0 = 0x1, // Layer0 is reserved for the main layer.
        Layer1 = 0x2,
        Layer2 = 0x4,
        Layer3 = 0x8,
        Layer4 = 0x10,
        Layer5 = 0x20,
        Layer6 = 0x40,
        Layer7 = 0x80,
        Layer8 = 0x100,
        Layer9 = 0x200,
        Layer10 = 0x400,
        Layer11 = 0x800,
        Layer12 = 0x1000,
        Layer13 = 0x2000,
        Layer14 = 0x4000,
        Layer15 = 0x8000,
        Layer16 = 0x10000,
        Layer17 = 0x20000,
        Layer18 = 0x40000,
        Layer19 = 0x80000,
        Layer20 = 0x100000,
        Layer21 = 0x200000,
        Layer22 = 0x400000,
        Layer23 = 0x800000,
        // All layers from Layer1 to Layer23 are reserved for user-defined layers.
        LayerAll = 0xFFFFFF,
        // Layers 24 and above are reserved for internal usage.
        Layer24 = 0x1000000,
        Layer25 = 0x2000000,
        Layer26 = 0x4000000,
        Layer27 = 0x8000000,
        Layer28 = 0x10000000,
        Layer29 = 0x20000000,
        Layer30 = 0x40000000,
        Layer31 = 0x80000000,
        ReservedLayerMask = 0xFF000000
    };
    Q_DECLARE_FLAGS(LayerFlags, LayerFlag)
    Q_FLAG(LayerFlags)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuick3DObject)

#endif // Q_QUICK3D_OBJECT_H
