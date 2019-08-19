#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQuick3D/QQuick3DViewport>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    QSurfaceFormat::setDefaultFormat(QQuick3DViewport::idealSurfaceFormat());

    QQmlApplicationEngine engine;
    engine.load(QUrl(QLatin1String("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
