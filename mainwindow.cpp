#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <float.h>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>
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
    headers << "名称" << "帧类型" << "数据字节" << "发送时间" << "优先级" << "节点数" << "最大长度";
    m_tableModel->setHorizontalHeaderLabels(headers);
    ui->tableView->setModel(m_tableModel);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "    background-color: #6E7BA5;"
        "    color: white;"
        "    font-weight: bold;"
        "    padding: 6px;"
        "    border: 1px solid #34495e;"
        "}"
        );

    // 初始化节点表格模型
    m_nodeModel = new QStandardItemModel(this);
    QStringList nodeHeaders;
    nodeHeaders << "节点名称" << "X坐标" << "Y坐标" << "使用消息数量" << "消息列表";
    m_nodeModel->setHorizontalHeaderLabels(nodeHeaders);
    ui->NodetableView->setModel(m_nodeModel);
    ui->NodetableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->NodetableView->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "    background-color: #6E7BA5;"
        "    color: white;"
        "    font-weight: bold;"
        "    padding: 6px;"
        "    border: 1px solid #34495e;"
        "}"
        );

    // 初始化网桥中继器表格模型
    m_bridgeModel = new QStandardItemModel(this);
    QStringList bridgeHeaders;
    bridgeHeaders << "设备类型" << "位置X" << "位置Y" << "连接网段" << "功能描述";
    m_bridgeModel->setHorizontalHeaderLabels(bridgeHeaders);
    ui->BridgetableWidget->setModel(m_bridgeModel);
    ui->BridgetableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->BridgetableWidget->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "    background-color: #6E7BA5;"
        "    color: white;"
        "    font-weight: bold;"
        "    padding: 6px;"
        "    border: 1px solid #34495e;"
        "}"
        );

    // 连接节点表格的双击信号
    connect(ui->NodetableView, &QTableView::doubleClicked, this, &MainWindow::onNodeDoubleClicked);

    // 初始化CAN ID分配表格模型
    m_idAllocationModel = new QStandardItemModel(this);
    QStringList idHeaders;
    idHeaders << "节点名称" << "消息名称" << "分配ID" ;
    //<< "二进制" << "帧类型"
    m_idAllocationModel->setHorizontalHeaderLabels(idHeaders);
    ui->idAllocationTableView->setModel(m_idAllocationModel);
    ui->idAllocationTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 初始化滤波器设计表格模型
    m_filterDesignModel = new QStandardItemModel(this);
    QStringList filterHeaders;
    filterHeaders << "模式" << "滤波器数量" << "滤波器ID" << "掩码/最大ID" << "说明";
    m_filterDesignModel->setHorizontalHeaderLabels(filterHeaders);
    ui->filterDesignTableView->setModel(m_filterDesignModel);
    ui->filterDesignTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 连接信号和槽
    connect(ui->CalculateButton, &QPushButton::clicked, this, &MainWindow::calculateOptimalBaudRate);
    ui->ChooseBaudcomboBox->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->ChooseBaudcomboBox, &QComboBox::customContextMenuRequested, this, &MainWindow::onBaudComboContextMenu);

    // 连接菜单栏
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::onActionOpen);
    connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::onActionExit);
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::onActionSave);
    connect(ui->actionExport, &QAction::triggered, this, &MainWindow::onActionExport);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    AddBaud *baudwindow = new AddBaud;
    baudwindow->show();
    connect(baudwindow, &AddBaud::dataAdded, this, &MainWindow::onBaudDataAdded, Qt::AutoConnection);
}

void MainWindow::onBaudDataAdded(const BaudRate &data)
{
    m_baudRateList.append(data);

    QList<QStandardItem*> rowItems;
    QString frameTypeText = (data.FrameType == 0) ? "标准帧" : "扩展帧";
    rowItems << new QStandardItem(data.Name);
    rowItems << new QStandardItem(frameTypeText);
    rowItems << new QStandardItem(QString::number(data.DataByte)+"byte");
    rowItems << new QStandardItem(QString::number(data.SendTime)+"ms");
    rowItems << new QStandardItem(QString::number(data.Priority));
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

    BaudRate originalData = m_baudRateList[row];

    AddBaud *editDialog = new AddBaud;
    editDialog->setBaudRateData(originalData);

    connect(editDialog, &AddBaud::dataAdded, this, [this, row](const BaudRate &updatedData) {
        m_baudRateList[row] = updatedData;

        QString frameTypeText = (updatedData.FrameType == 0) ? "标准帧" : "扩展帧";
        m_tableModel->item(row, 0)->setText(updatedData.Name);
        m_tableModel->item(row, 1)->setText(frameTypeText);
        m_tableModel->item(row, 2)->setText(QString::number(updatedData.DataByte)+"byte");
        m_tableModel->item(row, 3)->setText(QString::number(updatedData.SendTime)+"ms");
        m_tableModel->item(row, 4)->setText(QString::number(updatedData.Priority));
        m_tableModel->item(row, 5)->setText(QString::number(updatedData.Node));
        m_tableModel->item(row, 6)->setText(QString::number(updatedData.MaxLenth)+"m");
    }, Qt::AutoConnection);
    editDialog->show();
}

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

    // 波特率与最大长度对应表 (kbps -> m)
    QMap<float, int> baudRateToLength = {
        {1000, 30},
        {500, 100},
        {250, 250},
        {125, 500},
        {62.5, 1000},
        {50, 1300},
        {20, 3300},
        {10, 6700},
        {3, 10000}
    };

    QVector<float> commonBaudRates = {
        1000, 500, 250, 125, 62.5, 50, 20, 10, 3
    };

    // 计算所有信号中的最大长度
    float maxSignalLength = 0.0f;
    for (const BaudRate &data : m_baudRateList) {
        if (data.MaxLenth > maxSignalLength) {
            maxSignalLength = data.MaxLenth;
        }
    }

    qDebug() << "信号最大长度:" << maxSignalLength << "m";

    float totalBitsPerSecond = 0.0f;
    for (const BaudRate &data : m_baudRateList) {
        int frameBits = calculateFrameBits(data);
        float frequency = 1000.0f / data.SendTime;
        float bitsPerSecond = frameBits * frequency * data.Node;
        totalBitsPerSecond += bitsPerSecond;
    }

    qDebug() << "总比特率:" << totalBitsPerSecond << "bps";

    float requiredBaudRate = totalBitsPerSecond / (wantburden / 100.0f);

    // 检查期望波特率是否超过1000kbps (CAN总线最大支持)
    if (requiredBaudRate > 1000000) { // 1000kbps = 1000000bps
        QMessageBox::critical(this, "波特率超限",
                              QString("计算所需波特率 %1 kbps 超过CAN总线最大支持(1000 kbps)！\n\n"
                                      "建议措施：\n"
                                      "1. 降低期望负载率\n"
                                      "2. 减少通信数据量\n"
                                      "3. 降低通信频率\n"
                                      "4. 考虑使用CAN FD协议")
                                  .arg(requiredBaudRate / 1000.0, 0, 'f', 1));

        // 清空输出字段
        ui->RBaudlineEdit->clear();
        ui->PBaudlineEdit->clear();
        ui->BurdenlineEdit->clear();
        return;
    }

    ui->RBaudlineEdit->setText(QString::number(requiredBaudRate / 1000.0, 'f', 1) + " kbps");

    float recommendedBaudRate = 0;
    float optimalLoadRate = 100.0f;
    bool foundReasonable = false;

    // 寻找在期望负载率范围内的波特率
    for (float baudRate : commonBaudRates) {
        float loadRate = (totalBitsPerSecond / (baudRate * 1000)) * 100.0f;

        if (loadRate <= wantburden + 20 && loadRate >= qMax(5, wantburden - 20)) {
            if (!foundReasonable || qAbs(loadRate - wantburden) < qAbs(optimalLoadRate - wantburden)) {
                foundReasonable = true;
                optimalLoadRate = loadRate;
                recommendedBaudRate = baudRate;
            }
        }
    }

    // 如果没找到合理的，寻找负载率不超过100%的最佳选择
    if (!foundReasonable) {
        for (float baudRate : commonBaudRates) {
            float loadRate = (totalBitsPerSecond / (baudRate * 1000)) * 100.0f;
            float loadDiff = qAbs(loadRate - wantburden);
            float optimalDiff = qAbs(optimalLoadRate - wantburden);

            if (loadRate <= 100.0f && (loadDiff < optimalDiff || recommendedBaudRate == 0)) {
                optimalLoadRate = loadRate;
                recommendedBaudRate = baudRate;
            }
        }
    }

    // 如果还是没找到，选择负载率最低的
    if (recommendedBaudRate == 0) {
        for (float baudRate : commonBaudRates) {
            float loadRate = (totalBitsPerSecond / (baudRate * 1000)) * 100.0f;
            if (loadRate < optimalLoadRate) {
                optimalLoadRate = loadRate;
                recommendedBaudRate = baudRate;
            }
        }
    }

    // 检查网段长度限制
    int maxAllowedLength = baudRateToLength.value(recommendedBaudRate, 0);
    bool needsSegmentation = maxSignalLength > maxAllowedLength;

    QString segmentationInfo = "";
    if (needsSegmentation) {
        segmentationInfo = QString("\n\n⚠️ 网段划分警告：信号最大长度 %1m 超过推荐波特率 %2 kbps 的最大长度 %3m，需要划分网段！请在结构优化配置区域配置节点，软件自动划分网段！")
                               .arg(maxSignalLength)
                               .arg(recommendedBaudRate)
                               .arg(maxAllowedLength);
    } else {
        segmentationInfo = QString("\n\n✅ 网段长度检查：信号最大长度 %1m 在推荐波特率 %2 kbps 的最大长度 %3m 范围内，无需划分网段。")
                               .arg(maxSignalLength)
                               .arg(recommendedBaudRate)
                               .arg(maxAllowedLength);
    }

    ui->PBaudlineEdit->setText(QString::number(recommendedBaudRate, 'f', 1) + " kbps");
    ui->BurdenlineEdit->setText(QString::number(optimalLoadRate, 'f', 1) + "%");

    QString warning = "";
    if (optimalLoadRate > 100.0f) {
        warning = "\n\n⚠️ 警告：所有可用波特率都无法满足负载要求，当前为最低负载率！";
    } else if (optimalLoadRate > 80.0f) {
        warning = "\n\n⚠️ 注意：负载率较高，建议优化数据配置";
    }

    QString result = QString("波特率计算完成！\n\n"
                             "期望负载率: %1%\n"
                             "计算所需波特率: %2 kbps\n\n"
                             "推荐波特率: %3 kbps\n"
                             "实际负载率: %4%\n"
                             "信号最大长度: %5m\n"
                             "推荐波特率对应最大长度: %6m%7%8")
                         .arg(wantburden)
                         .arg(requiredBaudRate / 1000.0, 0, 'f', 1)
                         .arg(recommendedBaudRate, 0, 'f', 1)
                         .arg(optimalLoadRate, 0, 'f', 1)
                         .arg(maxSignalLength)
                         .arg(maxAllowedLength)
                         .arg(segmentationInfo)
                         .arg(warning);

    QMessageBox::information(this, "波特率计算", result);

    qDebug() << "期望负载率:" << wantburden << "%";
    qDebug() << "推荐波特率:" << recommendedBaudRate << "kbps";
    qDebug() << "实际负载率:" << optimalLoadRate << "%";
    qDebug() << "网段划分需要:" << (needsSegmentation ? "是" : "否");
}

int MainWindow::calculateFrameBits(const BaudRate &data)
{
    int baseBits = 0;
    if (data.FrameType == 0) {
        baseBits = 55 + 10 * data.DataByte;
    } else {
        baseBits = 80 + 10 * data.DataByte;
    }
    return baseBits;
}

void MainWindow::on_Next1Button_clicked()
{
    ui->tabWidget->setCurrentIndex(1);
}

void MainWindow::on_ChooseBaudcomboBox_currentIndexChanged(int index)
{
    if (ui->ChooseBaudcomboBox->itemText(index) == "自定义") {
        static int prevIndex = 0;
        prevIndex = (index == ui->ChooseBaudcomboBox->count()-1) ? prevIndex : index;

        ui->ChooseBaudcomboBox->blockSignals(true);
        bool ok;
        int baudRate = QInputDialog::getInt(this, "自定义波特率", "请输入波特率:", 9600, 1, 400000000, 1, &ok);

        if (ok) {
            bool exists = false;
            for (int i = 0; i < ui->ChooseBaudcomboBox->count()-1; i++) {
                if (ui->ChooseBaudcomboBox->itemText(i) == QString::number(baudRate)) {
                    exists = true;
                    break;
                }
            }
            if (exists) {
                ui->ChooseBaudcomboBox->setCurrentText(QString::number(baudRate));
            } else {
                ui->ChooseBaudcomboBox->insertItem(ui->ChooseBaudcomboBox->count()-1, QString::number(baudRate/1000)+" kbps");
                ui->ChooseBaudcomboBox->setCurrentText(QString::number(baudRate));
            }
        } else {
            ui->ChooseBaudcomboBox->setCurrentIndex(prevIndex);
        }
        ui->ChooseBaudcomboBox->blockSignals(false);
    }
}

void MainWindow::onBaudComboContextMenu(const QPoint &pos) {
    int index = ui->ChooseBaudcomboBox->currentIndex();
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
    int baudValue = 0;

    switch (baudindex) {
    case 0:
        baudValue = ui->PBaudlineEdit->text().replace(" kbps", "").toDouble() * 1000;
        break;
    case 1:
        baudValue = ui->RBaudlineEdit->text().replace(" kbps", "").toDouble() * 1000;
        break;
    default:
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
}

void MainWindow::on_AddNodebutton_clicked()
{
    QVector<QString> messageNames = getAvailableMessageNames();
    addNode *nodeDialog = new addNode (messageNames);

    connect(nodeDialog, &addNode::multipleNodesAdded, this, &MainWindow::onMultipleNodesAdded);
    connect(nodeDialog, &addNode::nodeDataAdded, this, &MainWindow::onNodeDataAdded);
    nodeDialog->show();
}

void MainWindow::onMultipleNodesAdded(const QVector<NodeInfo> &nodesData)
{
    m_nodeList.append(nodesData);
    refreshNodeTable();

    qDebug() << "批量添加" << nodesData.size() << "个节点";
    for (const NodeInfo &node : nodesData) {
        qDebug() << "节点:" << node.nodeName
                 << "位置: (" << node.xCoordinate << "," << node.yCoordinate << ")"
                 << "使用消息数:" << node.selectedMessages.size();
    }
}

QVector<QString> MainWindow::getAvailableMessageNames() const
{
    QVector<QString> names;
    for (const BaudRate &baud : m_baudRateList) {
        names.append(baud.Name);
    }
    return names;
}

void MainWindow::onNodeDataAdded(const NodeInfo &nodeData)
{
    m_nodeList.append(nodeData);
    refreshNodeTable();

    qDebug() << "添加节点:" << nodeData.nodeName
             << "位置: (" << nodeData.xCoordinate << "," << nodeData.yCoordinate << ")"
             << "选中消息数:" << nodeData.selectedMessages.size();

    for (const QString &messageName : nodeData.selectedMessages) {
        qDebug() << "  - " << messageName;
    }
}

void MainWindow::refreshNodeTable()
{
    m_nodeModel->removeRows(0, m_nodeModel->rowCount());

    for (const NodeInfo &node : m_nodeList) {
        QList<QStandardItem*> rowItems;

        rowItems << new QStandardItem(node.nodeName);
        rowItems << new QStandardItem(QString::number(node.xCoordinate, 'f', 2));
        rowItems << new QStandardItem(QString::number(node.yCoordinate, 'f', 2));
        rowItems << new QStandardItem(QString::number(node.selectedMessages.size()));

        QStringList messagesList = QStringList::fromVector(node.selectedMessages);
        QString messagesStr = messagesList.join(", ");
        rowItems << new QStandardItem(messagesStr);

        m_nodeModel->appendRow(rowItems);
    }

    ui->NodetableView->resizeColumnsToContents();
}

void MainWindow::onNodeDoubleClicked(const QModelIndex &index)
{
    if (index.isValid()) {
        editNode(index.row());
    }
}

void MainWindow::editNode(int row)
{
    if (row < 0 || row >= m_nodeList.size())
        return;

    NodeInfo originalData = m_nodeList[row];
    QVector<QString> messageNames = getAvailableMessageNames();

    addNode *editDialog = new addNode(messageNames);
    editDialog->setNodeData(originalData);

    connect(editDialog, &addNode::nodeDataAdded, this, [this, row](const NodeInfo &updatedData) {
        m_nodeList[row] = updatedData;
        refreshNodeTable();

        qDebug() << "更新节点:" << updatedData.nodeName
                 << "位置: (" << updatedData.xCoordinate << "," << updatedData.yCoordinate << ")"
                 << "使用消息数:" << updatedData.selectedMessages.size();
    });

    editDialog->show();
}

void MainWindow::on_slopeCalculateButton_clicked()
{
    if (!ui->slopeBaudRateSpinBox || !ui->slopeRiseTimeSpinBox ||
        !ui->slopeCapacitanceSpinBox || !ui->slopeCableLengthSpinBox) {
        QMessageBox::warning(this, "错误", "斜率控制UI组件未找到");
        return;
    }

    uint32_t baudrate = static_cast<uint32_t>(ui->slopeBaudRateSpinBox->value() * 1000);
    double targetRiseTime = ui->slopeRiseTimeSpinBox->value();
    double capacitance = ui->slopeCapacitanceSpinBox->value();
    double cableLength = ui->slopeCableLengthSpinBox->value();

    canopt1::SlopeControlInput input;
    input.baudrate = baudrate;
    input.targetRiseTimeNs = targetRiseTime;
    input.loadCapacitancePf = capacitance;
    input.cableLengthMeters = cableLength;
    input.maxRiseTimeRatio = 0.1;

    auto result = canopt1::CalculateSlopeControl(input);

    if (result.calculationSuccess) {
        if (ui->slopeResistorResultLabel) {
            ui->slopeResistorResultLabel->setText(QString::number(result.recommendedResistorOhm, 'f', 1) + " Ω");
        }

        if (ui->slopeActualRiseTimeLabel) {
            ui->slopeActualRiseTimeLabel->setText(QString::number(result.actualRiseTimeNs, 'f', 1) + " ns");
        }

        if (ui->slopeModeLabel) {
            ui->slopeModeLabel->setText(QString::fromStdString(result.recommendedMode));
        }

        if (ui->slopeStatusLabel) {
            QString status = result.isSuitable ? "✅ 满足时序要求" : "⚠️ 时序要求可能不满足";
            ui->slopeStatusLabel->setText(status);
        }

        QMessageBox::information(this, "计算成功",
                                 QString("斜率控制电阻计算完成！\n\n"
                                         "推荐电阻: %1 Ω\n"
                                         "推荐模式: %2\n"
                                         "实际上升时间: %3 ns")
                                     .arg(result.recommendedResistorOhm, 0, 'f', 1)
                                     .arg(QString::fromStdString(result.recommendedMode))
                                     .arg(result.actualRiseTimeNs, 0, 'f', 1));

    } else {
        QMessageBox::warning(this, "计算失败",
                             QString::fromStdString(result.statusMessage));

        if (ui->slopeStatusLabel) {
            ui->slopeStatusLabel->setText("❌ " + QString::fromStdString(result.statusMessage));
        }
    }
}

void MainWindow::on_calnetworkbutton_clicked()
{
    if (m_nodeList.isEmpty()) {
        qDebug() << "节点列表为空！";
        QMessageBox::warning(this, "错误", "请先添加节点数据！");
        return;
    }

    std::vector<IndustrialNet::Node> nodes;
    for (int i = 0; i < m_nodeList.size(); ++i) {
        IndustrialNet::Node n;
        n.id = i;
        n.x = m_nodeList[i].xCoordinate;
        n.y = m_nodeList[i].yCoordinate;
        nodes.push_back(n);
    }

    IndustrialNet::NetworkDesigner designer;
    IndustrialNet::DesignResult result = designer.designNetwork(nodes);

    // 更新网桥中继器表格
    updateBridgeTable(result);

    // 生成网络设计报告
    //QString report = generateNetworkReport(result, nodes);
    //ui->reportTextEdit->setPlainText(report);

    qDebug() << "=== 网络设计报告 ===";
    for (auto &seg : result.segments) {
        IndustrialNet::Node startNode = nodes[seg.node_ids.front()];
        IndustrialNet::Node endNode = nodes[seg.node_ids.back()];

        QString nodeIds;
        for (size_t i = 0; i < seg.node_ids.size(); ++i) {
            nodeIds += QString::number(seg.node_ids[i]);
            if (i != seg.node_ids.size() - 1) nodeIds += ", ";
        }

        qDebug() << "网段" << seg.id << ": 节点数 =" << seg.node_ids.size()
                 << "[" << nodeIds << "]";
        qDebug() << "  起点坐标 = (" << startNode.x << "," << startNode.y << "), "
                 << "终点坐标 = (" << endNode.x << "," << endNode.y << ")";
    }

    auto &dev = result.devices;

    qDebug() << "终端电阻位置:";
    for (auto &pos : dev.terminator_positions)
        qDebug() << "  (" << pos.first << "," << pos.second << ")";

    qDebug() << "中继器位置:";
    for (auto &pos : dev.repeater_positions)
        qDebug() << "  (" << pos.first << "," << pos.second << ")";

    qDebug() << "网桥位置:";
    for (auto &pos : dev.bridge_positions)
        qDebug() << "  (" << pos.first << "," << pos.second << ")";

    qDebug() << "节点接收显性电平:";
    for (auto &p : result.node_receive_ok)
        qDebug() << "  节点" << p.first << ":" << (p.second ? "OK" : "FAIL");

    qDebug() << "总体网络状态:" << (result.overall_ok ? "OK" : "FAIL");
    for (auto &log : result.logs)
        qDebug() << QString::fromStdString(log);

    QMessageBox::information(this, "网络设计完成",
                             QString("网络设计已完成！\n"
                                     "网段数量: %1\n"
                                     "终端电阻: %2个\n"
                                     "中继器: %3个\n"
                                     "网桥: %4个\n"
                                     "网络状态: %5")
                                 .arg(result.segments.size())
                                 .arg(dev.terminator_positions.size())
                                 .arg(dev.repeater_positions.size())
                                 .arg(dev.bridge_positions.size())
                                 .arg(result.overall_ok ? "正常" : "异常"));
}

// 更新中继器表格显示
void MainWindow::updateBridgeTable(const IndustrialNet::DesignResult& result)
{
    m_bridgeModel->removeRows(0, m_bridgeModel->rowCount());

    auto &dev = result.devices;

    // 添加终端电阻信息
    for (const auto& pos : dev.terminator_positions) {
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem("终端电阻");
        rowItems << new QStandardItem(QString::number(pos.first, 'f', 2));
        rowItems << new QStandardItem(QString::number(pos.second, 'f', 2));
        rowItems << new QStandardItem("网段末端");
        rowItems << new QStandardItem("匹配阻抗，防止信号反射");
        m_bridgeModel->appendRow(rowItems);
    }

    // 添加中继器信息
    for (const auto& pos : dev.repeater_positions) {
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem("中继器");
        rowItems << new QStandardItem(QString::number(pos.first, 'f', 2));
        rowItems << new QStandardItem(QString::number(pos.second, 'f', 2));

        // 查找中继器连接的网段
        QString connectedSegments;
        for (size_t i = 0; i < result.segments.size(); ++i) {
            const auto& seg = result.segments[i];

            // 修复 narrowing 问题：使用显式类型转换
            double startX = 0.0;
            double startY = 0.0;
            double endX = 0.0;
            double endY = 0.0;

            if (seg.node_ids.front() < m_nodeList.size()) {
                startX = static_cast<double>(m_nodeList[seg.node_ids.front()].xCoordinate);
                startY = static_cast<double>(m_nodeList[seg.node_ids.front()].yCoordinate);
            }

            if (seg.node_ids.back() < m_nodeList.size()) {
                endX = static_cast<double>(m_nodeList[seg.node_ids.back()].xCoordinate);
                endY = static_cast<double>(m_nodeList[seg.node_ids.back()].yCoordinate);
            }

            double distToStart = sqrt(pow(pos.first - startX, 2) + pow(pos.second - startY, 2));
            double distToEnd = sqrt(pow(pos.first - endX, 2) + pow(pos.second - endY, 2));

            if (distToStart < 10.0 || distToEnd < 10.0) { // 阈值可根据实际情况调整
                if (!connectedSegments.isEmpty()) connectedSegments += ", ";
                connectedSegments += QString::number(seg.id);
            }
        }

        rowItems << new QStandardItem(connectedSegments.isEmpty() ? "未知" : connectedSegments);
        rowItems << new QStandardItem("信号放大，延长传输距离");
        m_bridgeModel->appendRow(rowItems);
    }

    // 添加网桥信息
    for (const auto& pos : dev.bridge_positions) {
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem("网桥");
        rowItems << new QStandardItem(QString::number(pos.first, 'f', 2));
        rowItems << new QStandardItem(QString::number(pos.second, 'f', 2));

        // 网桥通常连接多个网段
        QString connectedSegments;
        int segmentCount = 0;
        for (size_t i = 0; i < result.segments.size() && segmentCount < 2; ++i) {
            const auto& seg = result.segments[i];

            // 修复 narrowing 问题：使用显式类型转换
            double startX = 0.0;
            double startY = 0.0;

            if (seg.node_ids.front() < m_nodeList.size()) {
                startX = static_cast<double>(m_nodeList[seg.node_ids.front()].xCoordinate);
                startY = static_cast<double>(m_nodeList[seg.node_ids.front()].yCoordinate);
            }

            double dist = sqrt(pow(pos.first - startX, 2) + pow(pos.second - startY, 2));
            if (dist < 15.0) { // 阈值可根据实际情况调整
                if (!connectedSegments.isEmpty()) connectedSegments += ", ";
                connectedSegments += QString::number(seg.id);
                segmentCount++;
            }
        }

        rowItems << new QStandardItem(connectedSegments.isEmpty() ? "多网段" : connectedSegments);
        rowItems << new QStandardItem("连接不同网段，实现网络扩展");
        m_bridgeModel->appendRow(rowItems);
    }

    ui->BridgetableWidget->resizeColumnsToContents();
}

// CAN ID分配与滤波器设计相关函数实现

std::vector<canopt2::CanNodeInfo> MainWindow::convertToCanNodes() const
{
    std::vector<canopt2::CanNodeInfo> nodes;

    for (const NodeInfo& qtNode : m_nodeList) {
        canopt2::CanNodeInfo node;
        node.nodeName = qtNode.nodeName.toStdString();

        for (const QString& messageName : qtNode.selectedMessages) {
            node.messageNames.push_back(messageName.toStdString());
        }

        nodes.push_back(node);
    }

    return nodes;
}

std::vector<canopt2::CanSignalInfo> MainWindow::convertToCanSignals() const
{
    std::vector<canopt2::CanSignalInfo> canSignals;  // 修改1：改为 canSignals

    for (const BaudRate& baud : m_baudRateList) {
        canopt2::CanSignalInfo signal;
        signal.messageName = baud.Name.toStdString();
        signal.priority = baud.Priority;
        signal.useExtendedId = (baud.FrameType == 1);

        canSignals.push_back(signal);  // 修改2：改为 canSignals
    }

    return canSignals;  // 修改3：改为 canSignals
}

//void MainWindow::on_allocateIdsButton_clicked()
//{
 //   if (m_nodeList.isEmpty() || m_baudRateList.isEmpty()) {
 //       QMessageBox::warning(this, "错误", "请先添加节点和消息数据！");
  //      return;
  //  }

 //   try {
  //      auto nodes = convertToCanNodes();
   //     auto canSignals = convertToCanSignals();  // 修改4：改为 canSignals

   //     bool useExtendedId = ui->extendedIdCheckBox->isChecked();
   //     uint32_t startId = static_cast<uint32_t>(ui->startIdSpinBox->value());

   //     m_idAllocationResults = canopt2::AllocateCanIds(nodes, canSignals, useExtendedId, startId);  // 修改5：改为 canSignals

   //     updateIdAllocationTable();

   //     QMessageBox::information(this, "成功", QString("已成功分配 %1 个CAN ID").arg(m_idAllocationResults.size()));

   // } catch (const std::exception& e) {
   //     QMessageBox::critical(this, "错误", QString("CAN ID分配失败: %1").arg(e.what()));
   // }
//}

void MainWindow::on_allocateIdsButton_clicked()
{
    if (m_nodeList.isEmpty() || m_baudRateList.isEmpty()) {
        QMessageBox::warning(this, "错误", "请先添加节点和消息数据！");
        return;
    }

    try {
        auto nodes = convertToCanNodes();
        auto canSignals = convertToCanSignals();

        // 详细调试输出
        qDebug() << "=== 详细调试信息 ===";
        qDebug() << "原始数据:";
        qDebug() << "m_baudRateList 数量:" << m_baudRateList.size();
        for (int i = 0; i < m_baudRateList.size(); ++i) {
            qDebug() << "信号" << i << ":" << m_baudRateList[i].Name
                     << "优先级:" << m_baudRateList[i].Priority;
        }

        qDebug() << "m_nodeList 数量:" << m_nodeList.size();
        for (int i = 0; i < m_nodeList.size(); ++i) {
            qDebug() << "节点" << i << ":" << m_nodeList[i].nodeName
                     << "消息数量:" << m_nodeList[i].selectedMessages.size();
            for (int j = 0; j < m_nodeList[i].selectedMessages.size(); ++j) {
                qDebug() << "  - 消息:" << m_nodeList[i].selectedMessages[j];
            }
        }

        qDebug() << "转换后数据:";
        qDebug() << "canSignals 数量:" << canSignals.size();
        for (size_t i = 0; i < canSignals.size(); ++i) {
            qDebug() << "信号" << i << ":" << QString::fromStdString(canSignals[i].messageName)
                     << "优先级:" << canSignals[i].priority;
        }

        bool useExtendedId = ui->extendedIdCheckBox->isChecked();
        uint32_t startId = static_cast<uint32_t>(ui->startIdSpinBox->value());

        qDebug() << "调用参数: useExtendedId =" << useExtendedId << "startId = 0x" << QString::number(startId, 16);

        m_idAllocationResults = canopt2::AllocateCanIds(nodes, canSignals, useExtendedId, startId);

        qDebug() << "分配结果数量:" << m_idAllocationResults.size();
        for (const auto& result : m_idAllocationResults) {
            qDebug() << "分配结果:" << QString::fromStdString(result.nodeName)
                     << "-" << QString::fromStdString(result.messageName)
                     << "- ID: 0x" << QString::number(result.allocatedId, 16);
        }

        updateIdAllocationTable();

        QMessageBox::information(this, "成功", QString("已成功分配 %1 个CAN ID").arg(m_idAllocationResults.size()));

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "错误", QString("CAN ID分配失败: %1").arg(e.what()));
    }
}

// 以下所有函数完全保持不变
void MainWindow::on_designFilterButton_clicked()
{
    if (m_idAllocationResults.empty()) {
        QMessageBox::warning(this, "错误", "请先分配CAN ID！");
        return;
    }

    try {
        std::vector<uint32_t> ids;
        for (const auto& result : m_idAllocationResults) {
            ids.push_back(result.allocatedId);
        }

        bool useExtendedId = ui->extendedIdCheckBox->isChecked();

        auto filterResult = canopt2::DesignCanFilter(ids, useExtendedId);

        updateFilterDesignTable(filterResult);

        QMessageBox::information(this, "成功",
                                 QString("滤波器设计完成！\n模式: %1\n滤波器数量: %2")
                                     .arg(QString::fromStdString(filterResult.mode))
                                     .arg(filterResult.filterCount));

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "错误", QString("滤波器设计失败: %1").arg(e.what()));
    }
}

void MainWindow::on_generateReportButton_clicked()
{
    if (m_idAllocationResults.empty()) {
        QMessageBox::warning(this, "错误", "没有可用的分配结果！");
        return;
    }

    try {
        std::string idReport = canopt2::GenerateIdAllocationReport(m_idAllocationResults);

        std::string fullReport = "=== CAN ID 分配报告 ===\n\n";
        fullReport += idReport;

        ui->reportTextEdit->setPlainText(QString::fromStdString(fullReport));

        QString fileName = QFileDialog::getSaveFileName(this, "保存报告", "can_id_report.txt", "文本文件 (*.txt)");
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream << QString::fromStdString(fullReport);
                file.close();
                QMessageBox::information(this, "成功", "报告已保存！");
            }
        }

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "错误", QString("生成报告失败: %1").arg(e.what()));
    }
}

void MainWindow::updateIdAllocationTable()
{
    m_idAllocationModel->removeRows(0, m_idAllocationModel->rowCount());

    for (const auto& result : m_idAllocationResults) {
        QList<QStandardItem*> rowItems;

        rowItems << new QStandardItem(QString::fromStdString(result.nodeName));
        rowItems << new QStandardItem(QString::fromStdString(result.messageName));
        rowItems << new QStandardItem(QString("0x%1").arg(result.allocatedId, 0, 16));

//QString binaryStr;
  //      uint32_t id = result.allocatedId;
  //      bool isExtended = ui->extendedIdCheckBox->isChecked();
 //       int bits = isExtended ? 29 : 11;

  //      for (int i = bits - 1; i >= 0; --i) {
  //          binaryStr.append((id >> i) & 1 ? '1' : '0');
  //      }
   //     rowItems << new QStandardItem(binaryStr);

   //     rowItems << new QStandardItem(isExtended ? "扩展帧" : "标准帧");

        m_idAllocationModel->appendRow(rowItems);
    }

    ui->idAllocationTableView->resizeColumnsToContents();
}

void MainWindow::updateFilterDesignTable(const canopt2::FilterDesignResult& result)
{
    m_filterDesignModel->removeRows(0, m_filterDesignModel->rowCount());

    QList<QStandardItem*> rowItems;

    rowItems << new QStandardItem(QString::fromStdString(result.mode));
    rowItems << new QStandardItem(QString::number(result.filterCount));
    rowItems << new QStandardItem(QString("0x%1").arg(result.filterId, 0, 16));
    rowItems << new QStandardItem(QString("0x%1").arg(result.maskOrMaxId, 0, 16));
    rowItems << new QStandardItem(QString::fromStdString(result.note));

    m_filterDesignModel->appendRow(rowItems);
    ui->filterDesignTableView->resizeColumnsToContents();
}



// 设置栏操作
void MainWindow::onActionOpen()
{

}

void MainWindow::onActionSave()
{
}

void MainWindow::onActionExit()
{
    close(); // 关闭应用程序
}

void MainWindow::onActionExport()
{
}

