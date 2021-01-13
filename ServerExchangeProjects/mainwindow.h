#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QWebSocketServer;
class QWebSocket;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void startServer(int port);

    void connectToDB();

    void addMsgToLog(const QString& str);

public slots:
    void slot_newConnection();

    void slot_disconnected();

    void slot_textMessageReceived(const QString& txtMsg);

    void slot_binaryMessageReceived(QByteArray binMsg);

private:
    void sendJson(QWebSocket* socket, const QJsonObject& obj);

    void sendNewMessage(const QString& msg, const QString& name, const QDateTime& time_message);

    void sendNewFile(const QString& fileName);

    void sendForAllClients(const QJsonObject& obj);

    void sendFileData(QWebSocket* socket ,const QString& fileName, const QByteArray& fileData);

private:
    Ui::MainWindow *ui;
    QWebSocketServer* webSocketServer = nullptr;
    QMap<QWebSocket*,QString> clients;
    QStringList files;
};
#endif // MAINWINDOW_H
