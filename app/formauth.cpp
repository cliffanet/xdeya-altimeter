#include "formauth.h"
#include "ui_formauth.h"

#include <QRegularExpressionValidator>

FormAuth::FormAuth(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FormAuth)
{
    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    flags &= ~ Qt::WindowContextHelpButtonHint;
    //flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);

    auto le = ui->leCode;
    le->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9A-Fa-f]*"), le));
    le->setMaxLength(4);

    ui->btnOk->setEnabled(false);
    connect(ui->btnOk,      &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->btnCancel,  &QPushButton::clicked, this, &QDialog::reject);
}

FormAuth::~FormAuth()
{
    delete ui;
}

uint16_t FormAuth::getCode()
{
    bool ok = false;
    uint16_t code = ui->leCode->text().toUShort(&ok, 16);
    return ok ? code : 0;
}


void FormAuth::on_leCode_textChanged(const QString &arg1)
{
    ui->btnOk->setEnabled(ui->leCode->text().length() >= 4);
}

