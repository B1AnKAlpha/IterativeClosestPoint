#include <QApplication>
#include "ElaApplication.h"
#include "app/appwindow.h"

int main(int argc, char *argv[])
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#else
    qputenv("QT_SCALE_FACTOR", "1.25");
#endif
#endif
    QApplication app(argc, argv);
    eApp->init();

    AppWindow window;
    window.show();

    return app.exec();
}
