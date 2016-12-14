#ifndef USERMANAGEMENT_H
#define USERMANAGEMENT_H

#include <QObject>
#include "applicationmodel.h"
#include <QTimer>
class UserManagement : public QObject
{
    Q_OBJECT
public:
    explicit UserManagement(QObject *home, QObject *shortcutArea, QObject *statusArea);

signals:

public slots:
    void slot_timerTest();
private:
    QObject *home;
    QObject *shortcutArea;
    QObject *statusArea;
    ApplicationModel *appModel;
    QTimer timerTest;
    QString currentLanguage;
};

#endif // USERMANAGEMENT_H
