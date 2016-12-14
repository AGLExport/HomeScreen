#include "usermanagement.h"
#include <QApplication>
#include <QDebug>
UserManagement::UserManagement(QObject *home, QObject *shortcutArea, QObject *statusArea) : QObject()
{
    this->home = home;
    this->appModel = home->findChild<ApplicationModel *>("ApplicationModel");
    this->shortcutArea = shortcutArea;
    this->statusArea = statusArea;
    this->currentLanguage = "en";
    connect(&timerTest, SIGNAL(timeout()), this, SLOT(slot_timerTest()));
    timerTest.setSingleShot(false);
    timerTest.start(5000);
}
void UserManagement::slot_timerTest()
{
    if(currentLanguage == "fr")
        currentLanguage = "en";
    else
        currentLanguage = "fr";
    appModel->changeLanguage(currentLanguage);
    QMetaObject::invokeMethod(home, "languageChanged");
    QMetaObject::invokeMethod(shortcutArea, "languageChanged", Q_ARG(QVariant, currentLanguage));
    QMetaObject::invokeMethod(statusArea, "languageChanged", Q_ARG(QVariant, currentLanguage));
    if(currentLanguage == "fr") {
        QLocale::setDefault(QLocale("fr_FR"));
        QMetaObject::invokeMethod(home, "showSign90", Q_ARG(QVariant, true));
        QMetaObject::invokeMethod(home, "showHello", Q_ARG(QVariant, "Bonjour José!"));
    } else {
        QLocale::setDefault(QLocale("en_US"));
        QMetaObject::invokeMethod(home, "showSign90", Q_ARG(QVariant, false));
        QMetaObject::invokeMethod(home, "showHello", Q_ARG(QVariant, "Hello José!"));
    }
}
