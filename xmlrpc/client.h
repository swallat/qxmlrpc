// vim:tabstop=4:shiftwidth=4:expandtab:cinoptions=(s,U1,m1
// Copyright (C) 2009 Dmytro Poplavskiy <dmitry.poplavsky@gmail.com>

#ifndef CLIENT_H
#define CLIENT_H

#include <qobject.h>
#include <QtNetwork>

#include "xmlrpc/variant.h"
class QAuthenticator;

namespace  xmlrpc {

/*!
 \class xmlrpc::Client client.h
 \brief The xmlrpc::Client class provides an implementation of the XML-RPC client.

 The class works asynchronously, so there are no blocking functions.

 Each XML-RPC request has unique identifier, which is returned by xmlrpc::Client::request()
 and is emited with done() and failed() signals.



\code
 client = new xmlrpc::Client(this);
 connect( client, SIGNAL(done( int, QVariant )),
          this, SLOT(processReturnValue( int, QVariant )) );
 connect( client, SIGNAL(failed( int, int, QString )),
          this, SLOT(processFault( int, int, QString )) );
 
 client->setHost( "localhost", 7777 );
 
 int requestId = client->request( "sum", x, y )
 
\endcode

 After the request is finished, done() or failed() signal
 will be emited with the request id and return value or fault information.
 */

class Client : public QObject {
Q_OBJECT
public:
	Client(QObject * parent = 0);
    Client(const QString & hostname, quint16 port = 80, QObject *parent = 0L);

    void setHost ( const QString & hostname, quint16 port = 80, QString path="/", bool useSSL = false );
    void setProxy ( const QNetworkProxy & proxy);
    void setUser ( const QString & userName, const QString & password = QString() );
    void setUrl( const QUrl &url);

    void setUserAgent( const QString & userAgent );

	virtual ~Client();

    void request( QList<Variant> params, QString methodName );

    /* overloaded methods */
    void request( QString methodName );
    void request( QString methodName, Variant param1 );
    void request( QString methodName, Variant param1, Variant param2 );
    void request( QString methodName, Variant param1, Variant param2, Variant param3 );
    void request( QString methodName, Variant param1, Variant param2, Variant param3, Variant param4 );


signals:
    //! request requestId is done with return value res
    void done( int requestId, QVariant res );

    //! request requestId is failed with fault code faultCode and fault description faultString
    void failed( int requestId, int faultCode, QString faultString );

    //! authenticationRequired signal passed from QNetworkAccessManager
    void authenticationRequired ( QNetworkReply * reply, QAuthenticator * );

    //! proxyAuthenticationRequired signal passed from QNetworkAccessManager
    void proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *);

    //! networkAccessibleChanged signal passed from QNetworkAccessManager
    void networkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible);

    //! sslError signal passed from QNetworkAccessManager
    void sslError(QNetworkReply * reply, const QList<QSslError> & errors);

protected slots:
    void finished(int id, bool error);

private:
	class Private;
	Private *d;
}; 

} // namespace

#endif // CLIENT_H


