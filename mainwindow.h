#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include "addbaud.h"

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

private:
    Ui::MainWindow *ui;

    AddBaud *m_addBaudDialog;
    QStandardItemModel *m_tableModel;
    QVector<BaudRate> m_baudRateList;
    void updateTableView();
    void editBaudRate(int row);

    float calculateBusLoad(float baudRate);  // 计算总线负载
    int calculateFrameBits(const BaudRate &data);  // 计算单帧比特数
};
#endif // MAINWINDOW_H
