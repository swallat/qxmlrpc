// vim:tabstop=4:shiftwidth=4:foldmethod=marker:expandtab:cinoptions=(s,U1,m1
// Copyright (C) 2009 Dmytro Poplavskiy <dmitry.poplavsky@gmail.com>

#include "xmlrpc/client.h"
#include "xmlrpc/request.h"
#include "xmlrpc/response.h"

#include <qhttp.h>
#include <qbuffer.h>
#include <QAuthenticator>

//#define XMLRPC_DEBUG

namespace  xmlrpc {

class Client::Private
{
public:
    QUrl url;
    QString hostName;
    quint16 port;
    QString path;
    bool useSSL;

    QString userName;
    QString password;

    QString userAgent;

    QNetworkAccessManager *accessManager;

    QAuthenticator proxyAuth;
};

/**
 * Constructs a XmlRPC client.
 */
Client::Client(QObject * parent)
: QObject( parent )
{
    d = new Private;
    d->port = 0;
    d->path = "/";
    d->userAgent = "QXMLRPC";
    d->useSSL = false;

    d->accessManager = new QNetworkAccessManager(this);

    connect( d->accessManager, SIGNAL( finished(QNetworkReply * ) ), SLOT( finished(QNetworkReply *) ) );

    connect( d->accessManager, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)),
             this, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *) ) );

    connect( d->accessManager, SIGNAL( authenticationRequired ( QNetworkReply *, QAuthenticator * ) ),
             this, SIGNAL( authenticationRequired ( QNetworkReply *, QAuthenticator * ) ) );

    connect( d->accessManager, SIGNAL( networkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility ) ),
             this, SIGNAL( networkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility)));

    connect( d->accessManager, SIGNAL( sslErrors(QNetworkReply*,QList<QSslError>)),
             this, SIGNAL(sslError(QNetworkReply*,QList<QSslError>)));
}

/**
 * Constructs a XmlRPC client for communication with XmlRPC
 * server running on host \a hostName \a port.
 */
Client::Client(const QString & hostName, quint16 port,  QObject * parent)
: QObject( parent )
{
    d = new Private;
    setHost( hostName, port );

    d->userAgent = "QXMLRPC";

    //important: dissconnect all connection from http in destructor,
    //otherwise crashes are possible when other parts of Client::Private
    //is deleted before http
    connect( d->accessManager, SIGNAL( finished(QNetworkReply * ) ), SLOT( finished(QNetworkReply *) ) );

    connect( d->accessManager, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)),
             this, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *) ) );

    connect( d->accessManager, SIGNAL( authenticationRequired ( QNetworkReply *, QAuthenticator * ) ),
             this, SIGNAL( authenticationRequired ( QNetworkReply *, QAuthenticator * ) ) );

    connect( d->accessManager, SIGNAL( networkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility ) ),
             this, SIGNAL( networkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility)));

    connect( d->accessManager, SIGNAL( sslErrors(QNetworkReply*,QList<QSslError>)),
             this, SIGNAL(sslError(QNetworkReply*,QList<QSslError>)));
}

/**
 * Destroys the XmlRPC client.
 */
Client::~Client()
{
    // it's necessary to delete QHttp instance before Private instance
    // to be sure Client slots will not be called with already deleted Private data
    delete d->accessManager;
    qDeleteAll( d->serverResponses );
    delete d;
}


/**
 * Sets the XML-RPC server that is used for requests to hostName
 * on port \a port and path \path.
 */
void Client::setHost( const QString & hostName, quint16 port, QString path, bool useSSL )
{
    d->hostName = hostName;
    d->port = port;
    d->path = path;
    d->useSSL = useSSL;
}

/**
 * Enables HTTP proxy support, using the proxy server host on port port.
 * username and password can be provided if the proxy server
 * requires authentication.
 */
void Client::setProxy( const QNetworkProxy &proxy )
{

#ifdef XMLRPC_DEBUG
    qDebug() << "xmlrpc client: set proxy" << host << port << userName << password;
#endif

    d->accessManager->setProxy(proxy);
}

/**
 * Set the user name userName and password password for XML-RPC
 * ( or http ) server that require authentication.
 */
void Client::setUser( const QString & userName, const QString & password )
{
    d->userName = userName;
    d->password = password;
}

void Client::setUrl(const QUrl & url)
{
    d->url = url;
}

/**
 * Set the user agent HTTP value instead of default "QXMLRPC"
 */
void Client::setUserAgent( const QString & userAgent )
{
    d->userAgent = userAgent;
}

/**
 * Call method methodName on server side with parameters list
 * params. Returns id of request, used in done() and failed()
 * signals.
 * 
 * The parameters order is changed in overloaded methods to
 * avoid situation when the only parameter is the list.
 * \code
 * QList<xmlrpc::Variant> parameter;
 * ...
 * int requestId = client->request( methodName, parameter );
 * \endcode
 * This leads to this method be called, with parameter treated
 * as parameters list. It's possible to fix this with next code:
 * \code
 * client->request( methodName, xmlrpc::Variant(parameter) );
 * \endcode
 * but to avoid such kind of bugs, the parameters order in
 * overloaded methods was changed.
 */
void Client::request( QList<Variant> params, QString methodName )
{
    QBuffer *outBuffer = new QBuffer;

    //TODO: Save data until finished slot received! Associate via objectName Property and static Message counter!
    QByteArray data = Request(methodName,params).composeRequest();

    QNetworkRequest request;
    request.setUrl(d->url);
    request.setHeader(QNetworkRequest::ContentLengthHeader, data.size());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");
    request.setRawHeader("User-Agent", d->userAgent);
    request.setRawHeader("Connection", "Keep-Alive");
    d->accessManager->post(request, data);

#ifdef XMLRPC_DEBUG
    qDebug() << "xmlrpc request(" << id << "): " << methodName;
    qDebug() << Variant(params).pprint();
#endif
    return;
    // TODO: DELETE if valid
/*
    QHttpRequestHeader header("POST",d->path);
    header.setContentLength( data.size() );
    header.setContentType("text/xml");
    header.setValue( "User-Agent", d->userAgent );
    header.setValue( "Connection", "Keep-Alive");


    if ( !d->userName.isEmpty() ) {
        QByteArray authData = QString(d->userName + ":" + d->password).toUtf8();
        authData = authData.toBase64();
        authData = QByteArray("Basic ")+authData;
        header.setValue("Authorization", authData.data() );
    }

    //header.setValue("Connection", "close");

    header.setValue("host",d->hostName);

    //d->http->setHost( d->hostName, d->port );

    int id = d->http->request( header, data, outBuffer );
    d->serverResponses[id] = outBuffer;
    d->methodNames[id] = methodName;
    d->http->close();



    return id;*/
}

/**
 * Call method methodName on server side with empty parameters
 * list. This is an overloaded member function, provided for
 * convenience.
 */
void Client::request( QString methodName )
{
    QList<xmlrpc::Variant> params;
    return request( params, methodName );
}

/**
 * Call method methodName on server side with one parameter.
 * This is an overloaded member function, provided for
 * convenience.
 */
void Client::request( QString methodName, Variant param1 )
{
    QList<xmlrpc::Variant> params;
    params << param1;
    return request( params, methodName );
}

/**
 * Call method methodName on server side with two parameters.
 * This is an overloaded member function, provided for
 * convenience.
 */
void Client::request( QString methodName, Variant param1, Variant param2 )
{
    QList<xmlrpc::Variant> params;
    params << param1 << param2;
    return request( params, methodName );
}

/**
 * Call method methodName on server side with three parameters.
 * This is an overloaded member function, provided for
 * convenience.
 */
void Client::request( QString methodName, Variant param1, Variant param2, Variant param3 )
{
    QList<xmlrpc::Variant> params;
    params << param1 << param2 << param3;
    return request( params, methodName );
}
/**
 * Call method methodName on server side with four parameters.
 * This is an overloaded member function, provided for
 * convenience.
 */
void Client::request( QString methodName, Variant param1, Variant param2, Variant param3, Variant param4 )
{
    QList<xmlrpc::Variant> params;
    params << param1 << param2 << param3 << param4;
    return request( params, methodName );
}

void Client::finished(int id, bool error)
{
    if ( !d->serverResponses.count(id) ) {
        return;
    }

#ifdef XMLRPC_DEBUG
    qDebug() << "request" <<  d->methodNames[id] <<  "finished, id=" << id << ", isError:" << error;
#endif

    if ( error ) {
        //if ( d->serverResponses.count(id) )

        QBuffer *buffer = d->serverResponses.take(id);
        delete buffer;


        emit failed(id, -32300, d->http->errorString() );
        return;
    }

    if ( d->serverResponses.count(id) ) {
        QBuffer *buffer = d->serverResponses.take(id);
        QByteArray buf = buffer->buffer();

        //qDebug() << "xml-rpc server response:\n" << QString(buf);

        Response response;

        QString errorMessage;
        if ( response.setContent( buf, &errorMessage ) ) {
            Q_ASSERT( !response.isNull() );

            if ( response.isFault() ) {
                qDebug() << "request failed:" << response.faultCode() << response.faultString();
                emit failed(id, response.faultCode(), response.faultString() );
            } else {
#ifdef XMLRPC_DEBUG
                qDebug() << response.returnValue().pprint();
#endif
                emit done( id, response.returnValue() );
            }

        } else {

#ifdef XMLRPC_DEBUG
            qDebug() << "incorrect xmlrpc response:" << errorMessage;
            qDebug() << QString(buf);
#endif
            emit failed(id, -32600, "Server error: Invalid xml-rpc. \nNot conforming to spec.");
        }
        delete buffer;

    }

}


} 


