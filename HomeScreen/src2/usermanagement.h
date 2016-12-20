#ifndef USERMANAGEMENT_H
#define USERMANAGEMENT_H

#define REAL_SERVER "ws://localhost:1212/api?token=hello"

#include <QObject>
#include "applicationmodel.h"
#include <QTimer>
#include <QPointF>
#include <QtWebSockets/QWebSocket>
#ifndef REAL_SERVER
#include <QtWebSockets/QWebSocketServer>
#endif
struct User {
    QString postal_address;
    QPointF loc;
    QString country;
    QString mail;
    QString city;
    QString graphEmail;
    QString graphPreferredLanguage;
    QString ccNumberMasked;
    QString ccExpYear;
    QString description;
    QString ccExpMonth;
    QStringList groups;
    QString last_name;
    QString ccNumber;
    QString house_identifier;
    QString phone;
    QString name;
    QString state;
    QString fax;
    QString common_name;
    QString postal_code;
    QString first_name;
    QString keytoken;
    QStringList graphActions;
    QString policy;
};

class UserManagement : public QObject
{
    Q_OBJECT
public:
    explicit UserManagement(QObject *root);

signals:

public slots:
    void connectWebsockets();
    void onConnected();
    void onClosed();
    void onTextMessageReceived(QString message);
    void onWebSocketError(QAbstractSocket::SocketError);
    void slot_sendData();
#ifndef REAL_SERVER
    void onServerNewConnection();
    void onServerClosed();
    void processBinaryMessage(QByteArray message);
    void processTextMessage(QString message);
    void serverSocketDisconnected();
    void slot_timerTest();
#endif
    void slot_turnOffRed();
    void slot_disconnect();
private:
    QObject *home;
    QObject *shortcutArea;
    QObject *statusArea;
    QObject *logo;
    QByteArray data;
    ApplicationModel *appModel;
    QWebSocket webSocket;
    QTimer timerRed;
    bool isRed;
    int sequence;
    bool jsonToMap(const QByteArray &buf, QVariantMap *map) const;
    bool mapToJson(const QVariantMap &map, QByteArray *json) const;
    bool jsonToList(const QByteArray &buf, QList<QVariant> *list) const;
    bool listToJson(const QList<QVariant> &list, QByteArray *json) const;
    void setUser(const User &user);
#ifndef REAL_SERVER
    QTimer timerTest;
    QWebSocket *pSocket;
    QWebSocketServer *webSocketServer;
    void launchServer();
#endif
};

#endif // USERMANAGEMENT_H
