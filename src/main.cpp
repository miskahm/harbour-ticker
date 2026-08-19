#include <QGuiApplication>
#include <QQuickView>
#include <sailfishapp.h>
#include "tickercontroller.h"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setApplicationName("harbour-ticker");
    app->setOrganizationName("harbour-ticker");

    qmlRegisterType<TickerController>("harbour.ticker", 1, 0, "TickerController");

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->setSource(SailfishApp::pathToMainQml());
    view->show();

    return app->exec();
}
