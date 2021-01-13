#ifndef DIALOGENTERCHAT_H
#define DIALOGENTERCHAT_H

#include <QDialog>

namespace Ui {
class DialogEnterChat;
}

class DialogEnterChat : public QDialog
{
    Q_OBJECT

public:
    explicit DialogEnterChat(QWidget *parent = nullptr);
    ~DialogEnterChat();

    QString getName();

private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    Ui::DialogEnterChat *ui;
};

#endif // DIALOGENTERCHAT_H
