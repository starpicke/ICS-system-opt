#pragma once
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPointF>
#include <vector>
#include "NetworkDesigner.h"

class NetworkView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit NetworkView(QWidget *parent = nullptr);

    // 设置网络数据（直接用 Designer 生成的坐标）
    void setNetworkData(const IndustrialNet::DesignResult &result,
                        const std::vector<IndustrialNet::Node> &nodes);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QGraphicsScene* scene_;
    std::vector<QPointF> nodes_;
    IndustrialNet::DesignResult result_;

    // 创建两点间圆弧
    QPainterPath createArc(const QPointF &a, const QPointF &b, double height = 30.0);

    // 缩放坐标到视图
    QPointF scalePoint(const QPointF &p, double minX, double minY, double scale, double margin);

    // 找最近节点
    QPointF nearestNode(const QPointF &p);

    // 避免设备碰撞的轻微偏移
    QPointF adjustDevicePosition(const QPointF &p, const std::vector<QPointF> &existing, double minDist = 10.0);
};
