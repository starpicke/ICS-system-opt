#include "addnode.h"
#include "ui_addnode.h"
#include <QMessageBox>

addNode::addNode(const QVector<QString> &availableMessages, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::addNode)
    , m_availableMessages(availableMessages)
{
    ui->setupUi(this);
    setupUI();
    populateMessageLists(availableMessages);

    // 连接信号槽
    connect(ui->OlistWidget, &QListWidget::itemChanged,
            this, &addNode::onSendListItemChanged);
    connect(ui->listWidget, &QListWidget::itemChanged,
            this, &addNode::onReceiveListItemChanged);
}

addNode::~addNode()
{
    delete ui;
}

void addNode::setupUI()
{
    // 设置坐标范围
    ui->XSpinBox->setRange(-10000.0, 10000.0);
    ui->YSpinBox->setRange(-10000.0, 10000.0);
    ui->XSpinBox->setDecimals(2);
    ui->YSpinBox->setDecimals(2);

    // 连接发送列表信号
    connect(ui->OlistWidget, &QListWidget::itemChanged, this, &addNode::onSendListItemChanged);

    // 连接接收列表信号
    connect(ui->listWidget, &QListWidget::itemChanged, this, &addNode::onReceiveListItemChanged);

    // 设置窗口属性
    setWindowTitle("配置节点");
}

void addNode::populateMessageLists(const QVector<QString> &availableMessages)
{
    ui->OlistWidget->clear();
    ui->listWidget->clear();

    for (const QString &messageName : availableMessages) {
        // 发送列表
        QListWidgetItem *sendItem = new QListWidgetItem(messageName);
        sendItem->setCheckState(Qt::Unchecked);
        sendItem->setData(Qt::UserRole, messageName);
        ui->OlistWidget->addItem(sendItem);

        // 接收列表
        QListWidgetItem *receiveItem = new QListWidgetItem(messageName);
        receiveItem->setCheckState(Qt::Unchecked);
        receiveItem->setData(Qt::UserRole, messageName);
        ui->listWidget->addItem(receiveItem);
    }
}

void addNode::setNodeData(const NodeInfo &nodeData)
{
    m_currentNodeData = nodeData;

    // 设置基本信息
    ui->NodeName->setText(nodeData.nodeName);
    ui->XSpinBox->setValue(nodeData.xCoordinate);
    ui->YSpinBox->setValue(nodeData.yCoordinate);

    // 设置发送消息列表
    for (int i = 0; i < ui->OlistWidget->count(); ++i) {
        QListWidgetItem *item = ui->OlistWidget->item(i);
        QString messageName = item->data(Qt::UserRole).toString();

        if (nodeData.selectedSendMessages.contains(messageName)) {
            item->setCheckState(Qt::Checked);
        } else {
            item->setCheckState(Qt::Unchecked);
        }
    }

    // 设置接收消息列表
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        QString messageName = item->data(Qt::UserRole).toString();

        if (nodeData.selectedReceiveMessages.contains(messageName)) {
            item->setCheckState(Qt::Checked);
        } else {
            item->setCheckState(Qt::Unchecked);
        }
    }
}

NodeInfo addNode::getNodeData() const
{
    NodeInfo nodeData;
    nodeData.nodeName = ui->NodeName->text().trimmed();
    nodeData.xCoordinate = ui->XSpinBox->value();
    nodeData.yCoordinate = ui->YSpinBox->value();

    // 收集选中的发送消息
    for (int i = 0; i < ui->OlistWidget->count(); ++i) {
        QListWidgetItem *item = ui->OlistWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            nodeData.selectedSendMessages.append(item->data(Qt::UserRole).toString());
        }
    }

    // 收集选中的接收消息
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            nodeData.selectedReceiveMessages.append(item->data(Qt::UserRole).toString());
        }
    }

    return nodeData;
}

void addNode::on_buttonBox_accepted()
{
    // 验证基础数据
    NodeInfo baseNodeData = getNodeData();
    if (baseNodeData.nodeName.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入节点名称！");
        return;
    }

    // 验证消息选择
    if (baseNodeData.selectedSendMessages.isEmpty() && baseNodeData.selectedReceiveMessages.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请至少选择一个发送或接收消息！");
        return;
    }

    if (ui->AutosetBox->isChecked()) {
        // === 自动生成矩阵节点模式 ===
        QVector<NodeInfo> matrixNodes = generateMatrixNodes();

        // 验证生成的节点数量
        int totalNodes = ui->HorizonSpinBox->value() * ui->VerticalspinBox->value();
        if (totalNodes == 0) {
            QMessageBox::warning(this, "参数错误", "节点数量不能为0！");
            return;
        }

        if (totalNodes > 1000) {
            QMessageBox::warning(this, "参数错误", "生成的节点数量过多（最大1000）！");
            return;
        }

        // 弹出确认对话框显示生成信息
        QString confirmMessage = QString("将生成 %1 个节点\n\n"
                                         "起始位置: (%2, %3)\n"
                                         "水平间距: %4, 垂直间距: %5\n"
                                         "布局: %6 × %7")
                                     .arg(totalNodes)
                                     .arg(ui->XSpinBox->value())
                                     .arg(ui->YSpinBox->value())
                                     .arg(ui->XINRSpinBox->value())
                                     .arg(ui->YINRSpinBox->value())
                                     .arg(ui->HorizonSpinBox->value())
                                     .arg(ui->VerticalspinBox->value());

        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "确认生成",
            confirmMessage,
            QMessageBox::Yes | QMessageBox::No
            );

        if (reply == QMessageBox::Yes) {
            emit multipleNodesAdded(matrixNodes);
            close();
        }
    } else {
        // === 单个节点模式 ===
        emit nodeDataAdded(baseNodeData);
        close();
    }
}

void addNode::on_buttonBox_rejected()
{
    close(); // 关闭窗口
}

QVector<NodeInfo> addNode::generateMatrixNodes()
{
    QVector<NodeInfo> nodes;

    // 获取基础节点信息（用户输入的原始节点）
    NodeInfo baseNode = getNodeData();
    QString baseName = baseNode.nodeName;

    // 获取矩阵参数
    double startX = ui->XSpinBox->value();           // 起始X坐标
    double startY = ui->YSpinBox->value();           // 起始Y坐标
    double xSpacing = ui->XINRSpinBox->value();      // 水平间距
    double ySpacing = ui->YINRSpinBox->value();      // 垂直间距
    int horizontalCount = ui->HorizonSpinBox->value();  // 水平节点数
    int verticalCount = ui->VerticalspinBox->value();   // 垂直节点数

    int nodeCounter = 1;  // 节点序号计数器

    // 按行生成节点（从上到下，从左到右）
    for (int row = 0; row < verticalCount; ++row) {
        for (int col = 0; col < horizontalCount; ++col) {
            NodeInfo newNode;

            // 设置节点名称：基础名称 + 序号
            newNode.nodeName = QString("%1%2").arg(baseName).arg(nodeCounter);

            // 计算坐标：起始坐标 + 列间距 × 列数，起始坐标 + 行间距 × 行数
            newNode.xCoordinate = startX + col * xSpacing;
            newNode.yCoordinate = startY + row * ySpacing;

            // 复制发送和接收消息列表（所有节点使用相同的消息配置）
            newNode.selectedSendMessages = baseNode.selectedSendMessages;
            newNode.selectedReceiveMessages = baseNode.selectedReceiveMessages;

            nodes.append(newNode);
            nodeCounter++;
        }
    }

    return nodes;
}

void addNode::onSendListItemChanged(QListWidgetItem *item)
{
    QString messageName = item->data(Qt::UserRole).toString();
    if (item->checkState() == Qt::Checked) {
        if (!m_currentNodeData.selectedSendMessages.contains(messageName)) {
            m_currentNodeData.selectedSendMessages.append(messageName);
        }
    } else {
        m_currentNodeData.selectedSendMessages.removeAll(messageName);
    }

    if (item->checkState() == Qt::Checked) {
        // 选中状态
        item->setForeground(Qt::black);
    } else {
        // 未选中状态，可以设置为灰色
        item->setForeground(Qt::gray);
    }
}

void addNode::onReceiveListItemChanged(QListWidgetItem *item)
{
    QString messageName = item->data(Qt::UserRole).toString();
    if (item->checkState() == Qt::Checked) {
        if (!m_currentNodeData.selectedReceiveMessages.contains(messageName)) {
            m_currentNodeData.selectedReceiveMessages.append(messageName);
        }
    } else {
        m_currentNodeData.selectedReceiveMessages.removeAll(messageName);
    }

    if (item->checkState() == Qt::Checked) {
        // 选中状态
        item->setForeground(Qt::black);
    } else {
        // 未选中状态，可以设置为灰色
        item->setForeground(Qt::gray);
    }
}
