#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWebSocket>
#include <dialogenterchat.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QClipboard>
#include <QApplication>
#include <QMimeData>
#include <QDataStream>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>

#include "../ServerExchangeProjects/CodesProjects.h"

#define TIRE "-----------------------------------------------------------------"

#define IP "localhost"
#define PORT 7654

QString createDownloadingFileName(const QString& fileName)
{
    QString dirPath = QApplication::applicationDirPath() + "/Downloads";
    if(!QDir(dirPath).exists())
    {
        QDir().mkpath(dirPath);
    }

    auto date = QDateTime::currentDateTime();

    return dirPath  + "/" + fileName;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("ClientExchangeProjects");
    setAcceptDrops(true);

    socket = new QWebSocket;

    connect(socket, &QWebSocket::connected, this, &MainWindow::slot_connected);
    connect(socket, &QWebSocket::disconnected, this, &MainWindow::slot_disconnected);
    connect(socket, &QWebSocket::textMessageReceived, this, &MainWindow::slot_TextMessageReceived);
    connect(ui->btnSendTxtMsg, &QPushButton::clicked, this, &MainWindow::slot_sendMyTextMessage);
    connect(ui->btnSendFile, &QPushButton::clicked, this, &MainWindow::slot_sendFile);
    connect(ui->btnGetFile, &QPushButton::clicked, this, &MainWindow::slot_downloadFile);
    connect(socket, &QWebSocket::binaryMessageReceived, this, &MainWindow::slot_binaryMessageReceived);

    QString url = QString("ws://") + IP + ":" + QString::number(PORT);
    qDebug() << "пытаюсь подключиться по адресу: " << url;

    socket->open(QUrl(url));

    DialogEnterChat* enterChat = new DialogEnterChat(this);
    int result = enterChat->exec();
    if(result == DialogEnterChat::Accepted)
    {
        name = enterChat->getName();
        ui->lblYourName->setText(name);
        sendMyName();
    }
    else
    {
        exit(2);
    }

    ui->listView->setModel(&modelListFiles); // устанавливаем модель 'список строк' в представление 'cписок'

}

MainWindow::~MainWindow()
{
    delete ui;
    delete socket;
}

void MainWindow::slot_connected()
{
    qDebug() << "connected";


    QJsonObject obj;
    obj.insert("type", CodesProjects::Messages);

    sendJson(obj);

    QJsonObject obj2;
    obj2.insert("type", CodesProjects::Files);

    sendJson(obj2);
}

void MainWindow::slot_TextMessageReceived(const QString &txtMsg)
{
    qDebug() << "TEXT massage received";

    QJsonDocument doc = QJsonDocument::fromJson(txtMsg.toUtf8());
    QJsonObject obj = doc.object();

    int type = obj.value("type").toInt();

    switch (type)
    {
    case CodesProjects::NewMessage:
    {
        QString message = getMessageFromJSON(obj);
        addMessage(message);
        break;
    }
    case CodesProjects::Messages:
    {
        QJsonArray messages = obj.value("messages").toArray();

        QString str;
        QJsonObject objMessage;

        for(const auto& message: messages)
        {
            QJsonObject objMessage = message.toObject();
            str = getMessageFromJSON(objMessage);
            addMessage(str);
        }

        break;
    }
    case CodesProjects::Files:
    {
        QJsonArray arrayFiles = obj.value("files").toArray();

        QJsonObject fileObj;

        QStringList listFiles;

        for(const auto& file : arrayFiles)
        {
            fileObj = file.toObject();
            listFiles << fileObj.value("file_name").toString();
        }

       modelListFiles.setStringList(listFiles);

        break;
    }
    case CodesProjects::NewFile:
    {
        QString fileName = obj.value("file_name").toString();

        int row = modelListFiles.rowCount();

        modelListFiles.insertRow(row);
        modelListFiles.setData(modelListFiles.index(row,0), fileName);
    }
    default:
    {
        qDebug() << "undefined type message = " << type;
        break;
    }
    }

}

void MainWindow::slot_disconnected()
{
    qDebug() << "disconnected";
    exit(1);
}

void MainWindow::slot_binaryMessageReceived(QByteArray binMsg)
{
    QString fileName;
    QByteArray fileData;

    QDataStream stream(&binMsg, QIODevice::ReadOnly);
    stream >> fileName >> fileData;

    qDebug() << " receive file " << fileName;

    QFile file(createDownloadingFileName(fileName));

    file.open(QFile::WriteOnly | QFile::Truncate); // открываем файл для записи (если он существует, то будет заменен)

    file.write(fileData);

    file.close();

    QMessageBox msgBox;
    msgBox.setWindowTitle("Файл скачан");
    msgBox.setText("Файл " + fileName + " успешно загружен с сервера в папку \'Downloads\'");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}

void MainWindow::slot_sendMyTextMessage()
{
    QString txtMsg = ui->editMyMsg->toPlainText();
    qDebug() << "my Text = " << txtMsg;

    if(txtMsg.isEmpty())
    {
        qDebug() << "my Text is empty. my text is not sended";
        return;
    }
    qDebug() << "send my TEXT massage";

    ui->editMyMsg->clear();

    QJsonObject obj;
    obj.insert("type", CodesProjects::NewMessage);
    obj.insert("message", txtMsg);

    sendJson(obj);
}

void MainWindow::slot_sendFile()
{
    QString filePath = ui->editPathFile->text();
    QFile file(filePath);
    if(file.open(QFile::ReadOnly))
    {
        int index = filePath.lastIndexOf('/');
        QString fileName;
        if(index != -1)
        {
            fileName = filePath.mid(index + 1);
        }
        else
        {
            fileName = filePath;
        }
        qDebug() << "send file " << fileName;

        QByteArray byteFile = file.readAll();

        QByteArray array;
        QDataStream stream(&array, QIODevice::WriteOnly);
        stream << fileName << byteFile;

        socket->sendBinaryMessage(array);
    }
    else
    {
        qDebug() << "It is not allow to open file " << filePath;
    }
    ui->editPathFile->clear();
}

void MainWindow::slot_downloadFile()
{
    auto selectedList = ui->listView->selectionModel()->selectedRows();
    if(!selectedList.isEmpty())
    {
        QString fileName = ui->listView->selectionModel()->currentIndex().data().toString();
        qDebug() << "try download file " << fileName;

        QJsonObject obj;
        obj.insert("type", CodesProjects::DownloadFile);
        obj.insert("file_name", fileName);

        sendJson(obj);
    }
}

void MainWindow::sendJson(const QJsonObject &obj)
{
    QJsonDocument doc(obj);
    socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void MainWindow::addMessage(const QString &msg)
{
    ui->editMessages->append(msg + "\n");
}

QString MainWindow::getMessageFromJSON(const QJsonObject &obj)
{
    QString str = QString(TIRE) + "\nСообщение от ";

    QString name = obj.value("user_name").toString();
    QString msg = obj.value("user_message").toString();
    QString dateTimeStr = obj.value("time_message").toString();

    str += "\'" +name +"\'" + " время: " + dateTimeStr + '\n' + msg + '\n'  + TIRE;
    return str;
}

void MainWindow::sendMyName()
{
    qDebug() << "send my name";
    QJsonObject obj;
    obj.insert("type", CodesProjects::UserName);
    obj.insert("user_name", name);

    sendJson(obj);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* pe)
{

    if(pe->mimeData()->hasFormat("text/uri-list"))
    {
        QList<QUrl> urlList = pe->mimeData()->urls();

        if(urlList.size() != 1)
        {
            return;
        }

        QString path = urlList[0].toLocalFile();

        QFileInfo fileInfo(path);

        if(fileInfo.isDir())  // запрет на перетаскивание папок
        {
            return;
        }

        qDebug() << "Перетаскивание разрешено";
        pe->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* pe)
{
    QList<QUrl> urlList = pe->mimeData()->urls();

    QString path = urlList[0].toLocalFile();

    ui->editPathFile->setText(path);
}

