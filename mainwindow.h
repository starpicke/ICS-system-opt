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
//#include "CanIdFilterLib.h"

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
    void onBaudDataAdded(const BaudRate &data); // 处理添加数据的槽函数

    void on_DeleteButton_clicked();
    void on_tableView_doubleClicked(const QModelIndex &index);

    void calculateOptimalBaudRate();  // 计算最佳波特率

    void on_Next1Button_clicked();

    void on_ChooseBaudcomboBox_currentIndexChanged(int index);

    void onBaudComboContextMenu(const QPoint &pos);

    void on_RunButton3_clicked();

    //添加节点
    void onNodeDataAdded(const NodeInfo &nodeData);
    void on_AddNodebutton_clicked();
    void onNodeDoubleClicked(const QModelIndex &index);
    void editNode(int row);
    void refreshNodeTable();

    void on_calnetworkbutton_clicked();

    void on_slopeCalculateButton_clicked();


    void onMultipleNodesAdded(const QVector<NodeInfo> &nodesData);
    //
    // 在 MainWindow 类中添加
private:
    // 添加报文ID分配相关的成员函数
    void allocateMessageIds();
    QMap<QString, int> getMessagePriorities() const;
    QVector<QPair<QString, QString>> generateNodeMessagePairs() const;

private:
    Ui::MainWindow *ui;

    //信号信息
    AddBaud *m_addBaudDialog;
    QStandardItemModel *m_tableModel;
    QVector<BaudRate> m_baudRateList;

    //节点信息
    QVector<NodeInfo> m_nodeList;  // 存储所有节点
    QStandardItemModel *m_nodeModel;
    QVector<QString> getAvailableMessageNames() const;

    void updateTableView();
    void editBaudRate(int row);

    float calculateBusLoad(float baudRate);  // 计算总线负载
    int calculateFrameBits(const BaudRate &data);  // 计算单帧比特数、


};
#endif // MAINWINDOW_H
