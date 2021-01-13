#include "dialogenterchat.h"
#include "ui_dialogenterchat.h"
#include <QDebug>

DialogEnterChat::DialogEnterChat(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogEnterChat)
{
    ui->setupUi(this);

    connect(ui->btnOk, &QPushButton::clicked, this, &DialogEnterChat::on_buttonBox_accepted);
    connect(ui->btnCancel, &QPushButton::clicked, this, &DialogEnterChat::on_buttonBox_rejected);

    setWindowTitle("Вход");
}

DialogEnterChat::~DialogEnterChat()
{
    delete ui;
}

QString DialogEnterChat::getName()
{
    return ui->lineEdit->text();
}

void DialogEnterChat::on_buttonBox_accepted()
{
    if(ui->lineEdit->text().isEmpty())
    {
        qDebug() << "Имя не может быть пустым";
        return;
    }

    this->accept();  // согласие
}

void DialogEnterChat::on_buttonBox_rejected()
{
    this->reject();  // отказ
}
