#ifndef FORMAUTH_H
#define FORMAUTH_H

#include <QDialog>

namespace Ui {
class FormAuth;
}

class FormAuth : public QDialog
{
    Q_OBJECT

public:
    explicit FormAuth(QWidget *parent = nullptr);
    ~FormAuth();

    uint16_t getCode();

private slots:
    void on_leCode_textChanged(const QString &arg1);

private:
    Ui::FormAuth *ui;
    QString code;
};

#endif // FORMAUTH_H
