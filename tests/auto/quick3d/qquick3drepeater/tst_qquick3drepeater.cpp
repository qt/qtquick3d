// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquick3drepeater : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquick3drepeater();

private slots:
    void setDelegateAfterModel();
};

tst_qquick3drepeater::tst_qquick3drepeater()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qquick3drepeater::setDelegateAfterModel()
{
    QQmlEngine engine;
    const QUrl url = testFileUrl("setDelegateAfterModel.qml");
    QQmlComponent component(&engine, url);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    // If the model was lost by setting the delegate, the count would be 0.
    QCOMPARE(object->property("count").toInt(), 3);

    QTest::ignoreMessage(
            QtWarningMsg,
            qPrintable(url.toString() + ":18:5: QML Repeater3D: Cannot retain explicitly set "
                                        "delegate on non-DelegateModel"));
    object->setProperty("useObjectModel", QVariant::fromValue<bool>(true));
    QCOMPARE(object->property("count").toInt(), 2);

    // The old model must not mess with the view anymore.
    QTest::failOnWarning(qPrintable(url.toString() + ":18:5: QML Repeater3D: Explicitly set delegate "
                                                     "is externally overridden"));
    QMetaObject::invokeMethod(object.data(), "plantDelegate");
    QCOMPARE(object->property("count").toInt(), 4);
}

QTEST_MAIN(tst_qquick3drepeater)

#include "tst_qquick3drepeater.moc"
