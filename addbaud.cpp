#include "addbaud.h"
#include "ui_addbaud.h"

AddBaud::AddBaud(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AddBaud)
{
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddBaud::handleAccepted);
}

AddBaud::~AddBaud()
{
    delete ui;
}

BaudRate AddBaud::getBaudRateData() const
{
    BaudRate data;

    // 从界面获取数据并填充结构体
    data.Name = ui->NameEdit->toPlainText();
    data.FrameType = static_cast<uint_least8_t>(ui->FrameTypecomboBox->currentIndex());
    data.DataByte = ui->DataNumspinBox->value();
    data.SendTime = ui->TdoubleSpinBox->value();
    data.Node = ui->NodespinBox->value();
    data.MaxLenth = ui->Lengthbox->value();

    return data;
}

void AddBaud::setBaudRateData(const BaudRate &data)
{
    // 根据您的实际控件名称修改
    ui->NameEdit->setText(data.Name);
    ui->FrameTypecomboBox->setCurrentIndex(data.FrameType);
    ui->DataNumspinBox->setValue(data.DataByte);
    ui->TdoubleSpinBox->setValue(data.SendTime);
    ui->NodespinBox->setValue(data.Node);
    ui->Lengthbox->setValue(data.MaxLenth);
}

void AddBaud::handleAccepted()
{
    BaudRate newData = getBaudRateData();
    emit dataAdded(newData);
    this->close();
}


void AddBaud::on_buttonBox_rejected()
{
    this->close();
}
