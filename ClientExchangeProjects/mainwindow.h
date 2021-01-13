#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#include <QStringListModel>

class QWebSocket;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void dragEnterEvent(QDragEnterEvent* pe) override;

    void dropEvent(QDropEvent* pe) override;

public slots:
    void slot_connected();

    void slot_TextMessageReceived(const QString& txtMsg);

    void slot_disconnected();

    void slot_binaryMessageReceived(QByteArray binMsg);

    void slot_sendMyTextMessage();

    void slot_sendFile();

    void slot_downloadFile();

private:
    void sendJson(const QJsonObject& obj);

    void addMessage(const QString& msg);

    QString getMessageFromJSON(const QJsonObject& obj);

    void sendMyName();


private:
    Ui::MainWindow *ui;
    QWebSocket* socket;  // веб-сокет
    QString name;        // имя пользователя
    QStringListModel modelListFiles;  // модель для хранения
};
#endif // MAINWINDOW_H
