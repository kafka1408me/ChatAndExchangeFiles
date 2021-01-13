#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWebSocketServer>
#include <QWebSocket>
#include <QtSql>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "CodesProjects.h"
#include <QDataStream>

#define DB_HOST       "192.168.1.70"     //"localhost"
#define DB_NAME       "projects"
#define DB_USERNAME   "postgres"
#define DB_PASS       "simplepass"

static MainWindow* mainWindow = nullptr;

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QString myMsg("%1 : %2");

    QDateTime currentTime = QDateTime::currentDateTime();
    QString msgToSend = myMsg.arg(currentTime.toString()).arg(msg);

    mainWindow->addMsgToLog(msgToSend);
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mainWindow = this;

    qInstallMessageHandler(myMessageOutput);

    setWindowTitle("MyExchangeProjectsServer");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::startServer(int port)
{
    webSocketServer = new QWebSocketServer("MyExchangeProjectsServer",
                                           QWebSocketServer::SslMode::NonSecureMode,
                                           this);

    if (!webSocketServer->listen(QHostAddress::Any, port))
    {
        qDebug() << "server is not started: " << webSocketServer->errorString();
    }
    else
    {
        qDebug() << "successfully start; and listen port = " << port;
        connect(webSocketServer, &QWebSocketServer::newConnection, this, &MainWindow::slot_newConnection);
    }

}

void MainWindow::connectToDB()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");

    db.setHostName(DB_HOST);
    db.setDatabaseName(DB_NAME);
    db.setUserName(DB_USERNAME);
    db.setPassword(DB_PASS);
    if (db.open())
    {
        qDebug() << "SUCCESS connection to database";

        QSqlQuery query;
        if(query.exec("SELECT file_name FROM files"))
        {
            qDebug() << "SUCCESS get all file names; count files = " << query.size();
            QSqlRecord rec = query.record();

            while(query.next())
            {
                files << query.value(rec.indexOf("file_name")).toString();
            }
        }
        else
        {
            qDebug() << "ERROR get all file names; error = " << query.lastError();
        }
    }
    else
    {
        qDebug() << "FAILURE connection to database: " << db.lastError();
    }


}

void MainWindow::addMsgToLog(const QString &str)
{
    ui->textEdit->append(str);
}

void MainWindow::slot_newConnection()
{
    QWebSocket* pSocket = webSocketServer->nextPendingConnection();
    connect(pSocket, &QWebSocket::disconnected, this, &MainWindow::slot_disconnected);
    connect(pSocket, &QWebSocket::textMessageReceived, this, &MainWindow::slot_textMessageReceived);
    connect(pSocket, &QWebSocket::binaryMessageReceived, this, &MainWindow::slot_binaryMessageReceived);
    clients.insert(pSocket, QString());
}

void MainWindow::slot_disconnected()
{
    QWebSocket* pSocket = qobject_cast<QWebSocket*>(sender());
    qDebug() << "client disconnected: " << pSocket << " ; name = " << clients[pSocket];
    clients.remove(pSocket);
    pSocket->deleteLater();
}

void MainWindow::slot_textMessageReceived(const QString &txtMsg)
{
    QWebSocket* pSocket = qobject_cast<QWebSocket*>(sender());
    qDebug() << "received TEXT message from client: " << pSocket << " ; name = " << clients[pSocket];

    QJsonDocument doc = QJsonDocument::fromJson(txtMsg.toUtf8());
    QJsonObject obj = doc.object();

    int type = obj.value("type").toInt();

    switch (type)
    {
    case CodesProjects::UserName:
    {
        QString name = obj.value("user_name").toString();
        qDebug() << "receive userName for " << pSocket << " ;name = " << name;
        clients[pSocket] = name;
        break;
    }
    case CodesProjects::NewMessage:
    {
        QString msg = obj.value("message").toString();
        QString name = clients[pSocket];
        QDateTime dateTime = QDateTime::currentDateTime();

        QSqlQuery query;
        query.prepare("INSERT INTO messages (user_name,user_message,time_message) "
                      "VALUES(:user_name,:user_message,:time_message)");
        query.bindValue(":user_name",name);
        query.bindValue(":user_message", msg);
        query.bindValue(":time_message", dateTime);

        if(query.exec())
        {
            qDebug() << "SUCCESS INSERT new message";
            sendNewMessage(msg, name, dateTime);
        }
        else
        {
            qDebug() << "ERROR INSERT new message; error " << query.lastError();
        }

        break;
    }
    case CodesProjects::Messages:
    {
        qDebug() << "client " << pSocket << " want to get all messages";
        QSqlQuery query;

        if(query.exec("SELECT * FROM messages"))
        {
            QSqlRecord rec = query.record();

            QJsonArray messages;
            QJsonObject message;

            while(query.next())
            {
                message["user_name"] = query.value(rec.indexOf("user_name")).toString();
                message["user_message"] = query.value(rec.indexOf("user_message")).toString();
                message["time_message"] = query.value(rec.indexOf("time_message")).toDateTime().toString();

                messages.append(message);
            }

            QJsonObject obj;
            obj.insert("type", type);
            obj.insert("messages", messages);

            sendJson(pSocket, obj);


            qDebug() << "SUCCESS get all messages";
        }
        else
        {
            qDebug() << "ERROR get all messages";
        }
        break;
    }
    case CodesProjects::Files:
    {
        if(!files.isEmpty())
        {
            QJsonArray filesArray;

            QJsonObject fileNameObj;

            for(const auto& fileName: files)
            {
                fileNameObj["file_name"] = fileName;
                filesArray.append(fileNameObj);
            }

            QJsonObject obj;
            obj.insert("type", CodesProjects::Files);
            obj.insert("files", filesArray);

            sendJson(pSocket, obj);
        }
        break;
    }
    case CodesProjects::DownloadFile:
    {
        QString fileName = obj.value("file_name").toString();
        qDebug() << clients[pSocket] << " want download file " << fileName;
        if(files.contains(fileName))
        {
            QSqlQuery query;
            query.prepare("SELECT file_data FROM files WHERE file_name=:file_name");
            query.bindValue(":file_name", fileName);

            query.exec();
            if(query.first())
            {
                QSqlRecord rec = query.record();

                QByteArray fileData = query.value(rec.indexOf("file_data")).toByteArray();

                sendFileData(pSocket, fileName, fileData);
            }
            else
            {
                qDebug() << "ERROR get file data for file " << fileName << " ;error = " << query.lastError();
            }
        }
        else
        {
            qDebug() << fileName << " is not saved in database";
        }
        break;
    }
    default:
        qDebug() << "undefined type message = " << type;
        break;
    }

}

void MainWindow::slot_binaryMessageReceived(QByteArray binMsg)
{
    QWebSocket* pSocket = qobject_cast<QWebSocket*>(sender());
    qDebug() << "received BINARY message from client: " << pSocket << " ; name = " << clients[pSocket];

    QString fileName;
    QByteArray fileData;

    QDataStream stream(&binMsg, QIODevice::ReadOnly);
    stream >> fileName >> fileData;

    qDebug() << clients[pSocket] << " sended file " << fileName;

    for(const auto file: files)
    {
        if(file == fileName)
        {
            fileName += "_1";
            break;
        }
    }

    QSqlQuery query;
    query.prepare("INSERT INTO files (file_name,file_data) VALUES(:file_name,:file_data)");
    query.bindValue(":file_name", fileName);
    query.bindValue(":file_data", fileData);

    if(query.exec())
    {
        qDebug() << "file " << fileName << " successfully save in database";
        files << fileName;
        sendNewFile(fileName);
    }
    else
    {
        qDebug() << "ERROR: cannot save file " << fileName << " ;error = " <<query.lastError();
    }

    //    QFile file(fileName);
    //    file.open(QFile::WriteOnly);

    //    file.write(dataFile);

}

void MainWindow::sendJson(QWebSocket *socket, const QJsonObject &obj)
{
    QJsonDocument doc(obj);
    socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void MainWindow::sendNewMessage(const QString &msg, const QString &name, const QDateTime &time_message)
{
    qDebug() << "send new message for all clients";
    QJsonObject obj;
    obj.insert("type", CodesProjects::NewMessage);

    obj.insert("user_message", msg);
    obj.insert("user_name", name);
    obj.insert("time_message", time_message.toString());

    sendForAllClients(obj);
}

void MainWindow::sendNewFile(const QString &fileName)
{
    QJsonObject obj;
    obj.insert("type", CodesProjects::NewFile);

    obj.insert("file_name", fileName);

    sendForAllClients(obj);
}

void MainWindow::sendForAllClients(const QJsonObject &obj)
{
    for(auto client: clients.keys())
    {
        sendJson(client, obj);
    }
}

void MainWindow::sendFileData(QWebSocket *socket, const QString &fileName, const QByteArray &fileData)
{
    qDebug() << "send file " << fileName << " to " << clients[socket];
    QByteArray array;
    QDataStream stream(&array, QIODevice::WriteOnly);
    stream << fileName << fileData;

    socket->sendBinaryMessage(array);
}

