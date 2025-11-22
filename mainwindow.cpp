#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 初始化ListView模型
    m_tableModel = new QStandardItemModel(this);
    QStringList headers;
    headers << "名称" << "帧类型" << "数据字节" << "发送时间" << "节点数" << "最大长度";
    m_tableModel->setHorizontalHeaderLabels(headers);
    ui->tableView->setModel(m_tableModel);  // 确保UI中有tableView
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "    background-color: #6E7BA5;"  // 背景色
        "    color: white;"               // 文字颜色
        "    font-weight: bold;"          // 字体加粗
        "    padding: 6px;"               // 内边距
        "    border: 1px solid #34495e;"  // 边框
        "}"
        );

    connect(ui->CalculateButton, &QPushButton::clicked, this, &MainWindow::calculateOptimalBaudRate);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    //qDebug() << "打开新的AddBaud对话框";
    AddBaud *baudwindow = new AddBaud;
    baudwindow->show();
    connect(baudwindow, &AddBaud::dataAdded, this, &MainWindow::onBaudDataAdded, Qt::AutoConnection);
}

// 添加处理数据的槽函数
void MainWindow::onBaudDataAdded(const BaudRate &data)
{
    m_baudRateList.append(data);

    // 创建一行数据，包含所有字段
    QList<QStandardItem*> rowItems;
    QString frameTypeText = (data.FrameType == 0) ? "标准帧" : "扩展帧";
    rowItems << new QStandardItem(data.Name);
    rowItems << new QStandardItem(frameTypeText);
    rowItems << new QStandardItem(QString::number(data.DataByte)+"byte");
    rowItems << new QStandardItem(QString::number(data.SendTime)+"ms");
    rowItems << new QStandardItem(QString::number(data.Node));
    rowItems << new QStandardItem(QString::number(data.MaxLenth)+"m");

    m_tableModel->appendRow(rowItems);
}


void MainWindow::on_DeleteButton_clicked()
{
    QModelIndex currentIndex = ui->tableView->currentIndex();
    if (currentIndex.isValid()) {
        int row = currentIndex.row();
        m_tableModel->removeRow(row);
        m_baudRateList.removeAt(row);
    }
}


void MainWindow::on_tableView_doubleClicked(const QModelIndex &index)
{
    editBaudRate(index.row());
}

void MainWindow::editBaudRate(int row)
{

    if (row < 0 || row >= m_baudRateList.size())
        return;

    // 获取要编辑的数据
    BaudRate originalData = m_baudRateList[row];

    // 创建编辑对话框
    AddBaud *editDialog = new AddBaud;

    // 设置当前数据到对话框
    editDialog->setBaudRateData(originalData);

    // 连接信号 - 使用lambda捕获行索引
    connect(editDialog, &AddBaud::dataAdded, this, [this, row](const BaudRate &updatedData) {
        //qDebug() << "收到编辑数据，行:" << row << "名称:" << updatedData.Name;
        // 更新数据向量
        m_baudRateList[row] = updatedData;

        // 更新表格显示
        QString frameTypeText = (updatedData.FrameType == 0) ? "标准帧" : "扩展帧";
        m_tableModel->item(row, 0)->setText(updatedData.Name);
        m_tableModel->item(row, 1)->setText(frameTypeText);
        m_tableModel->item(row, 2)->setText(QString::number(updatedData.DataByte)+"byte");
        m_tableModel->item(row, 3)->setText(QString::number(updatedData.SendTime)+"ms");
        m_tableModel->item(row, 4)->setText(QString::number(updatedData.Node));
        m_tableModel->item(row, 5)->setText(QString::number(updatedData.MaxLenth)+"m");
    }, Qt::AutoConnection);
    editDialog->show();
}


// 计算单帧比特数
int MainWindow::calculateFrameBits(const BaudRate &data)
{
    int baseBits = 0;
    if (data.FrameType == 0) {
        // 标准帧: 55 + 10 * data
        baseBits = 55 + 10 * data.DataByte;
    } else {
        // 扩展帧: 80 + 10 * data
        baseBits = 80 + 10 * data.DataByte;
    }
    return baseBits;
}

// 计算总线负载
float MainWindow::calculateBusLoad(float baudRate)
{
    if (baudRate <= 0) return 0.0f;

    float totalBitsPerSecond = 0.0f;

    // 计算所有数据的总比特率
    for (const BaudRate &data : m_baudRateList) {
        int frameBits = calculateFrameBits(data);

        // 计算该数据每秒发送的比特数
        // 注意：SendTime 是周期（ms），需要转换为频率（Hz）
        float frequency = 1000.0f / data.SendTime;  // Hz
        float bitsPerSecond = frameBits * frequency * data.Node;

        totalBitsPerSecond += bitsPerSecond;
    }

    // 计算负载率 = 总比特率 / 波特率
    float loadRate = (totalBitsPerSecond / baudRate) * 100.0f;
    return loadRate;
}

// 计算最佳波特率
void MainWindow::calculateOptimalBaudRate()
{
    if (m_baudRateList.isEmpty()) {
        qDebug() << "没有数据可计算";
        return;
    }

    // 常见的CAN波特率列表（单位：bps）
    QVector<float> commonBaudRates = {
        10000,     // 10 kbps
        20000,     // 20 kbps
        50000,     // 50 kbps
        100000,    // 100 kbps
        125000,    // 125 kbps
        250000,    // 250 kbps
        500000,    // 500 kbps
        800000,    // 800 kbps
        1000000    // 1 Mbps
    };

    float optimalBaudRate = 0;
    float minLoadRate = 100.0f;  // 初始设为100%

    // 遍历所有常见波特率，找到负载率最低且合理的
    for (float baudRate : commonBaudRates) {
        float loadRate = calculateBusLoad(baudRate);

        qDebug() << "波特率:" << baudRate << "bps, 负载率:" << loadRate << "%";

        // 选择负载率在30%-70%之间的最优值
        if (loadRate <= 70.0f && loadRate >= 5.0f) {
            if (qAbs(loadRate - 50.0f) < qAbs(minLoadRate - 50.0f)) {
                minLoadRate = loadRate;
                optimalBaudRate = baudRate;
            }
        }
    }

    // 如果没有找到理想值，选择负载率最低的
    if (optimalBaudRate == 0) {
        for (float baudRate : commonBaudRates) {
            float loadRate = calculateBusLoad(baudRate);
            if (loadRate < minLoadRate && loadRate <= 100.0f) {
                minLoadRate = loadRate;
                optimalBaudRate = baudRate;
            }
        }
    }

    // 显示结果
    if (optimalBaudRate > 0) {
        QString result = QString("推荐波特率: %1 kbps\n负载率: %2%")
                             .arg(optimalBaudRate / 1000.0)
                             .arg(minLoadRate, 0, 'f', 1);
        qDebug() << result;

        // 可以在界面上显示结果，比如使用 QMessageBox
        QMessageBox::information(this, "波特率计算", result);
    } else {
        QMessageBox::warning(this, "计算失败", "无法找到合适的波特率，请检查数据配置");
    }
}
