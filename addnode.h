#ifndef ADDNODE_H
#define ADDNODE_H

#include <QWidget>
#include <QListWidgetItem>

// 节点信息结构体
struct NodeInfo {
    QString nodeName;           // 节点名称
    double xCoordinate;         // X坐标
    double yCoordinate;         // Y坐标
    QVector<QString> selectedMessages;  // 选中的消息名称列表

    NodeInfo() : nodeName(""), xCoordinate(0.0), yCoordinate(0.0) {}
};

namespace Ui {
class addNode;
}

class addNode : public QWidget
{
    Q_OBJECT

public:
    explicit addNode(const QVector<QString> &availableMessages, QWidget *parent = nullptr);
    ~addNode();

    // 设置节点数据（用于编辑）
    void setNodeData(const NodeInfo &nodeData);

    // 获取节点数据
    NodeInfo getNodeData() const;

signals:
    void nodeDataAdded(const NodeInfo &nodeData);

    void multipleNodesAdded(const QVector<NodeInfo> &nodesData);

private slots:
    void onListItemChanged(QListWidgetItem *item);

    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

    void on_autosetBox_toggled(bool checked);

private:
    void setupUI();
    void populateMessageList(const QVector<QString> &availableMessages);

private:
    Ui::addNode *ui;
    QVector<QString> m_availableMessages;
    NodeInfo m_currentNodeData;

    QVector<NodeInfo> generateMatrixNodes();
};

#endif // ADDNODE_H
