#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QIcon>
#include <QQuickStyle>


int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);

    QIcon::setThemeName("default");
    auto style =
#if defined(Q_OS_MACOS)
        QLatin1String("macOS");
#elif defined(Q_OS_IOS)
        QLatin1String("iOS");
#elif defined(Q_OS_WINDOWS)
        QLatin1String("Windows");
#else
        QLatin1String("Universal");
#endif
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
        QQuickStyle::setStyle(style);

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/page/main.qml"));

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection
    );
    engine.load(url);

    return app.exec();
}
