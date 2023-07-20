#include "automintnotification.h"
#include "automintmodel.h"

#include "ui_automintnotification.h"

#include <QPushButton>

AutomintNotification::AutomintNotification(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AutomintNotification),
    lelantusModel(nullptr)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Anonymize"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Dismiss"));

    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
}

AutomintNotification::~AutomintNotification()
{
    delete ui;
}

void AutomintNotification::setModel(WalletModel *model)
{
    if (model) {
        lelantusModel = model->getLelantusModel();

        if (!lelantusModel) {
            return;
        }

        auto automintModel = lelantusModel->getAutoMintModel();
        if (!automintModel) {
            return;
        }

        connect(this, &AutomintNotification::ackMintAll, automintModel, &AutoMintModel::ackMintAll);
    }
}

bool AutomintNotification::close()
{
    Q_EMIT ackMintAll(AutoMintAck::NotEnoughFund, 0, QString());
    return QDialog::close();
}

void AutomintNotification::accept()
{
    Q_EMIT ackMintAll(AutoMintAck::AskToMint, 0, QString());
    QDialog::accept();
}

void AutomintNotification::reject()
{
    Q_EMIT ackMintAll(AutoMintAck::UserReject, 0, QString());
    QDialog::reject();
}