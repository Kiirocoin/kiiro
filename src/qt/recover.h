#ifndef KIIRO_RECOVER_H
#define KIIRO_RECOVER_H

#include <QDialog>
#include <QThread>

namespace Ui {
    class Recover;
}

class Recover : public QDialog
{
    Q_OBJECT

public:
    explicit Recover(QWidget *parent = 0);
    ~Recover();

    static bool askRecover(bool& newWallet);

Q_SIGNALS:
    void stopThread();

private Q_SLOTS:
    void on_createNew_clicked();
    void on_recoverExisting_clicked();
    void on_usePassphrase_clicked();

private:
    void setCreateNew();

private:
    Ui::Recover *ui;
    QThread *thread;
};

#endif //KIIRO_RECOVER_H
