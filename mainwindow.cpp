#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <float.h>
#include <QInputDialog>
#include "CANBitTiming.h"
#include "OptionalCanFeatures.hpp"

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
    ui->ChooseBaudcomboBox->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->ChooseBaudcomboBox, &QComboBox::customContextMenuRequested, this, &MainWindow::onBaudComboContextMenu);
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

// 处理数据的槽函数
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

//双击编辑
void MainWindow::on_tableView_doubleClicked(const QModelIndex &index)
{
    editBaudRate(index.row());
}


//编辑具体实现
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

// 点击运行，计算波特率
void MainWindow::calculateOptimalBaudRate()
{
    if (m_baudRateList.isEmpty()) {
        QMessageBox::warning(this, "计算失败", "没有数据可计算");
        return;
    }

    int wantburden = ui->BurdenSetBox->value();
    if (wantburden <= 0 || wantburden > 100) {
        QMessageBox::warning(this, "输入错误", "负载率应在1-100%之间");
        return;
    }

    // 常见的CAN波特率列表
    QVector<float> commonBaudRates = {
        10000, 20000, 50000, 100000, 125000, 250000, 500000, 800000, 1000000
    };

    // 1. 计算总比特率
    float totalBitsPerSecond = 0.0f;
    for (const BaudRate &data : m_baudRateList) {
        int frameBits = calculateFrameBits(data);
        float frequency = 1000.0f / data.SendTime;
        float bitsPerSecond = frameBits * frequency * data.Node;
        totalBitsPerSecond += bitsPerSecond;
    }

    qDebug() << "总比特率:" << totalBitsPerSecond << "bps";

    // 2. 计算所需波特率（基于期望负载率）
    float requiredBaudRate = totalBitsPerSecond / (wantburden / 100.0f);
    ui->RBaudlineEdit->setText(QString::number(requiredBaudRate / 1000.0, 'f', 1) + " kbps");

    // 3. 优先选择在合理负载率范围内的波特率
    float recommendedBaudRate = 0;
    float optimalLoadRate = 100.0f;
    bool foundReasonable = false;

    // 首先寻找负载率在期望范围内的
    for (float baudRate : commonBaudRates) {
        float loadRate = (totalBitsPerSecond / baudRate) * 100.0f;

        // 如果找到在期望负载率±20%范围内的，优先选择
        if (loadRate <= wantburden + 20 && loadRate >= qMax(5, wantburden - 20)) {
            if (!foundReasonable || qAbs(loadRate - wantburden) < qAbs(optimalLoadRate - wantburden)) {
                foundReasonable = true;
                optimalLoadRate = loadRate;
                recommendedBaudRate = baudRate;
            }
        }
    }

    // 如果没有找到合理范围的，选择最接近期望负载率的
    if (!foundReasonable) {
        for (float baudRate : commonBaudRates) {
            float loadRate = (totalBitsPerSecond / baudRate) * 100.0f;
            float loadDiff = qAbs(loadRate - wantburden);
            float optimalDiff = qAbs(optimalLoadRate - wantburden);

            if (loadRate <= 100.0f && (loadDiff < optimalDiff || recommendedBaudRate == 0)) {
                optimalLoadRate = loadRate;
                recommendedBaudRate = baudRate;
            }
        }
    }

    // 如果还是没有找到（所有波特率都超载），选择负载率最低的
    if (recommendedBaudRate == 0) {
        for (float baudRate : commonBaudRates) {
            float loadRate = (totalBitsPerSecond / baudRate) * 100.0f;
            if (loadRate < optimalLoadRate) {
                optimalLoadRate = loadRate;
                recommendedBaudRate = baudRate;
            }
        }
    }

    // 4. 显示结果
    ui->PBaudlineEdit->setText(QString::number(recommendedBaudRate / 1000.0, 'f', 1) + " kbps");
    ui->BurdenlineEdit->setText(QString::number(optimalLoadRate, 'f', 1) + "%");

    // 5. 警告信息
    QString warning = "";
    if (optimalLoadRate > 100.0f) {
        warning = "\n\n⚠️ 警告：所有可用波特率都无法满足负载要求，当前为最低负载率！";
    } else if (optimalLoadRate > 80.0f) {
        warning = "\n\n⚠️ 注意：负载率较高，建议优化数据配置";
    }

    // 6. 显示结果
    QString result = QString("波特率计算完成！\n\n"
                             "期望负载率: %1%\n"
                             "计算所需波特率: %2 kbps\n\n"
                             "推荐波特率: %3 kbps\n"
                             "实际负载率: %4%%5")
                         .arg(wantburden)
                         .arg(requiredBaudRate / 1000.0, 0, 'f', 1)
                         .arg(recommendedBaudRate / 1000.0, 0, 'f', 1)
                         .arg(optimalLoadRate, 0, 'f', 1)
                         .arg(warning);

    QMessageBox::information(this, "波特率计算", result);

    qDebug() << "期望负载率:" << wantburden << "%";
    qDebug() << "推荐波特率:" << recommendedBaudRate << "bps";
    qDebug() << "实际负载率:" << optimalLoadRate << "%";
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

void MainWindow::on_Next1Button_clicked()
{
    ui->tabWidget->setCurrentIndex(1);
}


// 功能3部分：选择波特率
void MainWindow::on_ChooseBaudcomboBox_currentIndexChanged(int index)
{
    if (ui->ChooseBaudcomboBox->itemText(index) == "自定义") {
        // 保存之前的选择
        static int prevIndex = 0;
        prevIndex = (index == ui->ChooseBaudcomboBox->count()-1) ? prevIndex : index;

        //堵塞
        ui->ChooseBaudcomboBox->blockSignals(true);
        // 弹窗输入
        bool ok;
        int baudRate = QInputDialog::getInt(this, "自定义波特率", "请输入波特率:", 9600, 1, 400000000, 1, &ok);

        if (ok) {
            // 检查是否已存在相同波特率
            bool exists = false;
            for (int i = 0; i < ui->ChooseBaudcomboBox->count()-1; i++) {
                if (ui->ChooseBaudcomboBox->itemText(i) == QString::number(baudRate)) {
                    exists = true;
                    break;
                }
            }
            if (exists) {
                // 如果已存在，直接选择该选项
                ui->ChooseBaudcomboBox->setCurrentText(QString::number(baudRate));
            } else {
                // 不存在才添加
                ui->ChooseBaudcomboBox->insertItem(ui->ChooseBaudcomboBox->count()-1, QString::number(baudRate/1000)+" kbps");
                ui->ChooseBaudcomboBox->setCurrentText(QString::number(baudRate));
            }
        } else {
            // 取消或关闭，恢复之前选择
            ui->ChooseBaudcomboBox->setCurrentIndex(prevIndex);
        }
        //停止堵塞
        ui->ChooseBaudcomboBox->blockSignals(false);
    }
}
void MainWindow::onBaudComboContextMenu(const QPoint &pos) {
    int index = ui->ChooseBaudcomboBox->currentIndex();
    // 只允许删除自定义的选项（不是预定义的和不是"自定义"项）
    if (index >= 0 && index < ui->ChooseBaudcomboBox->count()-1) {
        QMenu menu;
        QAction *deleteAction = menu.addAction("删除");
        if (menu.exec(ui->ChooseBaudcomboBox->mapToGlobal(pos)) == deleteAction) {
            ui->ChooseBaudcomboBox->removeItem(index);
        }
    }
}



void MainWindow::on_RunButton3_clicked()
{
    int baudindex = ui->ChooseBaudcomboBox->currentIndex();
    QString baudText = ui->ChooseBaudcomboBox->currentText().replace(" kbps","");
    int baudValue = 0;  // 先定义

    switch (baudindex) {
    case 0:  // 理想波特率
        baudValue = ui->PBaudlineEdit->text().replace(" kbps", "").toDouble() * 1000;
        break;
    case 1:  // 另一个波特率
        baudValue = ui->RBaudlineEdit->text().replace(" kbps", "").toDouble() * 1000;
        break;
    default: // 自定义的值
        baudValue = baudText.toInt()*1000;
        break;
    }

    int clock = ui->CLKbox->value()*1000000;

    canopt::BitTimingInput input;
    input.systemClock = clock;
    input.targetBaudRate = baudValue;
    input.maxErrorPercent = 5.0;

    qDebug() << "计算成功" << baudValue << clock;
    auto result = canopt::CalculateBitTiming(input);

    // 4. 在Qt界面上显示结果
    if (result.calculationSuccess) {

        ui->BRPtext->setText(QString::number(result.BRP));
        ui->SJWtext->setText(QString::number(result.SJW));
        ui->TSEG1text->setText(QString::number(result.TSEG1));
        ui->TSEG2text->setText(QString::number(result.TSEG2));
        ui->btrRegisterLabel->setText("0x" + QString::number(result.btrRegister, 16).toUpper());
        ui->statuslabel->setText("✅计算成功 - 误差: " + QString::number(result.errorPercent, 'f', 2) + "%");
    } else {
        ui->statuslabel->setText("❌错误: " + QString::fromStdString(result.statusMessage));
    }


    //ui->BRPtext->setPlainText(QString::number(baudValue));
}
