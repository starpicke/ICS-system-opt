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
#include "NetworkView.h"
#include <QVBoxLayout>
#include "ConfigManager.h"
#include "ConfigData.h"

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

    // 添加公共getter方法
    float getMaxAllowedBaudRate() const { return m_maxAllowedBaudRate; }
    bool isBaudRateCalculated() const { return m_baudRateCalculated; }

private slots:
    void onBaudDataAdded(const BaudRate &data);
    void onImportConfigClicked();
    void onExportConfigClicked();
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
    void on_DeleteNodeButton_clicked();
    void onNodeDoubleClicked(const QModelIndex &index);
    void editNode(int row);
    void refreshNodeTable();

    void on_calnetworkbutton_clicked();

    void slopeCalculateButton_clicked();

    void onMultipleNodesAdded(const QVector<NodeInfo> &nodesData);

    // CAN ID分配与滤波器设计相关槽函数
    void allocateAndDesignButton_clicked();

    // 设置栏
    void onActionOpen();
    void onActionNew();
    void onActionExit();
    void onActionExport();

    void on_pushButton_clicked();

    void on_Next2Button_clicked();

    void on_ClearNodeButton_clicked();

private:
    Ui::MainWindow *ui;  // 🎯 将 ui 放在最前面

    // 🎯 按照初始化顺序排列成员变量
    float m_maxAllowedBaudRate;
    bool m_baudRateCalculated;
    float m_maxAllowedSegmentLength;

    // 配置管理
    config::ConfigManager configManager;
    config::SystemConfig currentConfig;

    // 网络视图
    NetworkView* networkView_;
    QVBoxLayout* networkLayout_;

private:
    void UpdateConfigFromGUI();
    config::SystemConfig CollectConfigFromGUI();
    void ApplyConfigToGUI(const config::SystemConfig& config);

private:
    void allocateMessageIds();
    void designMessageFilter();
    void updateIdAllocationTable();
    void updateFilterDesignTable(const std::vector<std::pair<std::string, canopt2::FilterDesignResult>>& filterResults);

    // 数据转换函数
    std::vector<canopt2::CanNodeInfo> convertToCanNodes() const;
    std::vector<canopt2::CanSignalInfo> convertToCanSignals() const;
    std::vector<std::pair<std::string, canopt2::FilterDesignResult>> designFiltersForAllNodes() const;
    std::string generateCompleteReport(const std::vector<std::pair<std::string, canopt2::FilterDesignResult>>& filterResults) const;

private:
    double calculateTotalBitRate();
    double calculateActualLoadPercent();
    double calculateSamplingPoint();
    double calculateMaxCableLength();
    double calculateTimingError();
    double calculatePropagationDelay();
    double extractRiseTimeFromLabel(const QString& labelText);
    double extractResistanceFromLabel(const QString& labelText);
    QPixmap captureNetworkView();
    QString saveNetworkScreenshot();
    QString m_currentScreenshotPath;

    void refreshBaudRateTable();  // 🎯 添加缺失的方法声明

private:
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

    // CAN ID分配结果
    std::vector<canopt2::IdAllocationResult> m_idAllocationResults;
    QStandardItemModel *m_idAllocationModel;
    QStandardItemModel *m_filterDesignModel;

    QVector<QString> getAvailableMessageNames() const;
    void updateTableView();
    void editBaudRate(int row);
    float calculateBusLoad(float baudRate);
    int calculateFrameBits(const BaudRate &data);

    int time_N;
    int maxalllength;
};

#endif // MAINWINDOW_H
