#include "addnode.h"
#include "ui_addnode.h"

addNode::addNode(const QVector<QString> &availableMessages, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::addNode)
    , m_availableMessages(availableMessages)
{
    ui->setupUi(this);
    setupUI();
    populateMessageList(availableMessages);
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

    connect(ui->listWidget, &QListWidget::itemChanged, this, &addNode::onListItemChanged);

    // 设置窗口属性
    setWindowTitle("配置节点");
}

void addNode::populateMessageList(const QVector<QString> &availableMessages)
{
    ui->listWidget->clear();

    for (const QString &messageName : availableMessages) {
        QListWidgetItem *item = new QListWidgetItem(messageName);
        item->setCheckState(Qt::Unchecked);
        item->setData(Qt::UserRole, messageName); // 存储原始消息名称
        ui->listWidget->addItem(item);
    }
}

void addNode::setNodeData(const NodeInfo &nodeData)
{
    m_currentNodeData = nodeData;

    // 设置基本信息
    ui->NodeName->setText(nodeData.nodeName);
    ui->XSpinBox->setValue(nodeData.xCoordinate);
    ui->YSpinBox->setValue(nodeData.yCoordinate);

    // 设置选中的消息
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        QString messageName = item->data(Qt::UserRole).toString();

        if (nodeData.selectedMessages.contains(messageName)) {
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

    // 收集选中的消息
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            nodeData.selectedMessages.append(item->data(Qt::UserRole).toString());
        }
    }

    return nodeData;
}

void addNode::onListItemChanged(QListWidgetItem *item)
{
    // 这里可以添加一些实时处理逻辑
    // 比如根据选择状态改变项的颜色等
    if (item->checkState() == Qt::Checked) {
        // 选中状态
        item->setForeground(Qt::black);
    } else {
        // 未选中状态，可以设置为灰色
        item->setForeground(Qt::gray);
    }
}

void addNode::on_buttonBox_accepted()
{
    NodeInfo nodeData = getNodeData();

    // 验证数据
    if (nodeData.nodeName.isEmpty()) {
        // 可以添加错误提示
        return;
    }

    emit nodeDataAdded(nodeData);
    close(); // 关闭窗口
}


void addNode::on_buttonBox_rejected()
{
    close(); // 关闭窗口
}

