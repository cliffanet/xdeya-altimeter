#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QIcon>
#include <QQuickStyle>
#include <QtWebView>

#include "apphnd.h"
#include "netprocess.h"
#include "jmpinfo.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);
    qmlRegisterType<AppHnd>("com.xdeya.app", 1, 0, "AppHnd");
    qmlRegisterType<JmpInfo>("com.xdeya.jmpinfo", 1, 0, "JmpInfo");
    qmlRegisterType<JmpInfo>("com.xdeya.trkinfo", 1, 0, "TrkInfo");

    QtWebView::initialize();

    QIcon::setThemeName("default");
    auto style =
#if defined(Q_OS_MACOS)
        //QLatin1String("macOS"); // тут не работает ProgressBar в indeterminate режиме
        QLatin1String("Fusion");
#elif defined(Q_OS_IOS)
        //QLatin1String("iOS");
        QLatin1String("Fusion");
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

    AppHnd apphnd;
    engine.rootContext()->setContextProperty("app", &apphnd);
    engine.rootContext()->setContextProperty("jmpInfo", apphnd.getJmp());

    engine.load(url);

    return app.exec();
}
