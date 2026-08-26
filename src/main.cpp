#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QObject>
#include <QQmlContext>
#include <QQuickView>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>
#include <sailfishapp.h>
#include "tickercontroller.h"

namespace {
QString debugLogPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/debug.log");
}

void fileMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    static const QString path = debugLogPath();
    const char lvl = type == QtDebugMsg ? 'D' : type == QtInfoMsg ? 'I'
                  : type == QtWarningMsg ? 'W' : type == QtCriticalMsg ? 'C' : 'F';
    QFile f(path);
    if (f.open(QIODevice::Append | QIODevice::WriteOnly)) {
        QTextStream ts(&f);
        ts << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
           << ' ' << lvl << ' ' << msg.toLocal8Bit().constData() << '\n';
    }
}
} // namespace

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setApplicationName("harbour-ticker");
    app->setOrganizationName("harbour-ticker");

    qInstallMessageHandler(fileMessageHandler);

    // One shared instance for pages + cover (context property, like harbour-sysmetrics)
    TickerController controller;

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->rootContext()->setContextProperty(QStringLiteral("ticker"), &controller);

    const QUrl source = SailfishApp::pathToMainQml();
    qInfo() << "startup: appname=" << app->applicationName()
            << "mainqml=" << source.toString()
            << "fileExists=" << QFile::exists(source.toLocalFile());
    view->setSource(source);
    view->show();

    QObject::connect(view.data(), &QQuickView::statusChanged, [](QQuickView::Status s) {
        qInfo() << "view status changed:" << static_cast<int>(s);
    });
    QQuickView *v = view.data();
    QTimer::singleShot(5000, v, [v]() {
        qInfo() << "t+5s: status=" << static_cast<int>(v->status())
                << "rootObject=" << (v->rootObject() != nullptr);
    });

    return app->exec();
}
