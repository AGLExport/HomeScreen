#ifndef USERMANAGEMENT_H
#define USERMANAGEMENT_H

#include <QObject>
#include "applicationmodel.h"
#include <QTimer>
#include <QtWebSockets/QWebSocket>
class UserManagement : public QObject
{
    Q_OBJECT
public:
    explicit UserManagement(QObject *home, QObject *shortcutArea, QObject *statusArea);

signals:

public slots:
    void slot_timerTest();
    void onConnected();
    void onClosed();
    void onTextMessageReceived(QString message);
private:
    QObject *home;
    QObject *shortcutArea;
    QObject *statusArea;
    ApplicationModel *appModel;
    QTimer timerTest;
    QString currentLanguage;
    QWebSocket webSocket;
    void connectWebsockets(const QUrl &url);
};

#endif // USERMANAGEMENT_H
