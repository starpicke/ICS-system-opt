#ifndef ADDBAUD_H
#define ADDBAUD_H
/*本文件负责记录并传递信息数据*/
#include <QWidget>

namespace Ui {
class AddBaud;
}

struct BaudRate{
    QString Name; //信息名称
    uint_least8_t FrameType; //标准帧还是扩展帧
    int DataByte;//数据场长度
    float SendTime;//周期
    int Node;//节点数
    float MaxLenth;//最远距离
    int Priority;
};

class AddBaud : public QWidget
{
    Q_OBJECT

public:
    explicit AddBaud(QWidget *parent = nullptr);
    ~AddBaud();

    BaudRate getBaudRateData() const;
    void setBaudRateData(const BaudRate &data);

signals:
    // 添加信号，用于在数据准备好时通知其他组件
    void dataAdded(const BaudRate &data);

private slots:
    void handleAccepted();

    void on_buttonBox_rejected();

private:
    Ui::AddBaud *ui;
};

#endif // ADDBAUD_H
