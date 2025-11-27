#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include "addbaud.h"
#include "addnode.h"
#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QTextEdit>
#include "NetworkDesigner.h"
#include "CanIdFilterLib.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void onBaudDataAdded(const BaudRate &data);

    void on_DeleteButton_clicked();
    void on_tableView_doubleClicked(const QModelIndex &index);

    void calculateOptimalBaudRate();

    void on_Next1Button_clicked();

    void on_ChooseBaudcomboBox_currentIndexChanged(int index);

    void onBaudComboContextMenu(const QPoint &pos);

    void on_RunButton3_clicked();

    // 添加节点相关
    void onNodeDataAdded(const NodeInfo &nodeData);
    void on_AddNodebutton_clicked();
    void onNodeDoubleClicked(const QModelIndex &index);
    void editNode(int row);
    void refreshNodeTable();

    void on_calnetworkbutton_clicked();

    void on_slopeCalculateButton_clicked();

    void onMultipleNodesAdded(const QVector<NodeInfo> &nodesData);

    // CAN ID分配与滤波器设计相关槽函数
    void on_allocateIdsButton_clicked();
    void on_designFilterButton_clicked();
    void on_generateReportButton_clicked();

    // 设置栏
    void onActionOpen();
    void onActionSave();
    void onActionExit();
    void onActionExport();

private:
    // CAN ID分配相关的成员函数
    void allocateMessageIds();
    void designMessageFilter();
    void updateIdAllocationTable();
    void updateFilterDesignTable(const canopt2::FilterDesignResult& result);

    // 数据转换函数
    std::vector<canopt2::CanNodeInfo> convertToCanNodes() const;
    std::vector<canopt2::CanSignalInfo> convertToCanSignals() const;

private:
    Ui::MainWindow *ui;

    // 信号信息
    AddBaud *m_addBaudDialog;
    QStandardItemModel *m_tableModel;
    QVector<BaudRate> m_baudRateList;

    // 节点信息
    QVector<NodeInfo> m_nodeList;
    QStandardItemModel *m_nodeModel;

     // 网桥中继器表格模型
    QStandardItemModel *m_bridgeModel;
    void updateBridgeTable(const IndustrialNet::DesignResult& result);
   // QString generateNetworkReport(const IndustrialNet::DesignResult& result,
                                  //const std::vector<IndustrialNet::Node>& nodes);

    // CAN ID分配结果
    std::vector<canopt2::IdAllocationResult> m_idAllocationResults;
    QStandardItemModel *m_idAllocationModel;
    QStandardItemModel *m_filterDesignModel;

    QVector<QString> getAvailableMessageNames() const;
    void updateTableView();
    void editBaudRate(int row);
    float calculateBusLoad(float baudRate);
    int calculateFrameBits(const BaudRate &data);
};

#endif // MAINWINDOW_H
