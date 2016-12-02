#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <unistd.h>
#include <vector>
#include <QQmlContext>
#include <libhomescreen.hpp>    // use libhomescreen
#include "calledbyqml.h"


#define FULLSCREEN 1    // assume 1 is "fullscreen"


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    /*  use libhomescreen
        To use libhomescreen, add library path in project file(.pro)
    */
    LibHomeScreen *mp_libHomeScreen;
    mp_libHomeScreen = new LibHomeScreen();

    QQmlApplicationEngine engine;
    CalledByQml call_hsa;
    engine.rootContext()->setContextProperty("hsa",&call_hsa);

    int appcategory = 0 ;

    bool enabled = true;
    enabled = mp_libHomeScreen->renderAppToAreaAllowed(appcategory, FULLSCREEN);
    if(enabled)
    {
        engine.addImportPath(QStringLiteral(":/imports"));
        engine.addImportPath(QStringLiteral(":/dummyimports"));

        engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

        std::vector<int> surfaceIdList;
        int pid = getpid();
        //maybe we can't call this function...
        //surfaceIdList = mp_libHomeScreen->getAllSurfacesOfProcess(pid);
        if(surfaceIdList.empty())
        {
            qDebug("surface list is empty");
        }
        else
        {
            qDebug("surface list is contained");
            // it will be implemented as soon as possible
            //mp_libHomeScreen->renderSurfaceToArea(surfaceIdList.at(0),FULLSCREEN);
        }
    }
    else
    {
        qDebug("renderAppToAreaAllowed is denied");
        delete mp_libHomeScreen;
        return 0;
    }

    app.exec();
    delete mp_libHomeScreen;
    return 0;
}