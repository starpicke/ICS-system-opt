#include "NetworkView.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <algorithm>

NetworkView::NetworkView(QWidget *parent)
    : QGraphicsView(parent),
    scene_(new QGraphicsScene(this))
{
    setScene(scene_);
    setRenderHint(QPainter::Antialiasing);
}

void NetworkView::setNetworkData(const IndustrialNet::DesignResult &result,
                                 const std::vector<IndustrialNet::Node> &nodes)
{
    result_ = result;
    nodes_.clear();
    for (const auto &n : nodes)
        nodes_.push_back(QPointF(n.x, n.y));

    scene_->clear();
    viewport()->update();
}

QPointF NetworkView::scalePoint(const QPointF &p, double minX, double minY, double scale, double margin)
{
    return QPointF((p.x() - minX) * scale + margin, (p.y() - minY) * scale + margin);
}

QPainterPath NetworkView::createArc(const QPointF &a, const QPointF &b, double height)
{
    QPainterPath path(a);
    QPointF mid = (a + b) / 2.0;
    QPointF diff = b - a;
    QPointF normal(-diff.y(), diff.x());
    double len = std::hypot(normal.x(), normal.y());
    if (len < 1e-6) len = 1.0;
    QPointF unitN = normal / len;
    QPointF ctrl = mid + unitN * height;
    path.quadTo(ctrl, b);
    return path;
}

QPointF NetworkView::nearestNode(const QPointF &p)
{
    double minDist = 1e9;
    QPointF nearest;
    for (auto &n : nodes_) {
        double d = std::hypot(n.x() - p.x(), n.y() - p.y());
        if (d < minDist) { minDist = d; nearest = n; }
    }
    return nearest;
}

QPointF NetworkView::adjustDevicePosition(const QPointF &p, const std::vector<QPointF> &existing, double minDist)
{
    QPointF pos = p;
    double step = 5.0;
    int maxAttempts = 50;

    for (int i = 0; i < maxAttempts; ++i) {
        bool collision = false;
        for (const auto &e : existing) {
            if (std::hypot(pos.x() - e.x(), pos.y() - e.y()) < minDist) {
                collision = true;
                break;
            }
        }
        if (!collision) break;
        pos += QPointF(step, step);
    }
    return pos;
}

void NetworkView::paintEvent(QPaintEvent *event)
{
    QGraphicsView::paintEvent(event);
    if (nodes_.empty()) return;

    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing);

    // 计算缩放
    double minX = nodes_[0].x(), maxX = nodes_[0].x();
    double minY = nodes_[0].y(), maxY = nodes_[0].y();
    for (auto &pt : nodes_) {
        minX = qMin(minX, pt.x()); maxX = qMax(maxX, pt.x());
        minY = qMin(minY, pt.y()); maxY = qMax(maxY, pt.y());
    }

    double margin = 50.0;
    double viewWidth  = width()  - 2*margin;
    double viewHeight = height() - 2*margin;
    double scaleX = viewWidth / (maxX - minX + 1);
    double scaleY = viewHeight / (maxY - minY + 1);
    double scale = qMin(scaleX, scaleY);

    std::vector<QPointF> existingDevices;

    // ===== 绘制网段 + 中继器 =====
    painter.setPen(QPen(Qt::blue, 2));
    painter.setBrush(Qt::NoBrush);

    for (const auto &seg : result_.segments) {
        for (size_t i = 1; i < seg.node_ids.size(); ++i) {
            QPointF a = scalePoint(nodes_[seg.node_ids[i-1]], minX, minY, scale, margin);
            QPointF b = scalePoint(nodes_[seg.node_ids[i]],   minX, minY, scale, margin);

            // 找在当前节点对之间的中继器
            std::vector<QPointF> repeaters;
            for (auto &r : result_.devices.repeater_positions) {
                QPointF rp = scalePoint(QPointF(r.first, r.second), minX, minY, scale, margin);
                double d1 = std::hypot(rp.x()-a.x(), rp.y()-a.y());
                double d2 = std::hypot(rp.x()-b.x(), rp.y()-b.y());
                double segLen = std::hypot(b.x()-a.x(), b.y()-a.y());
                if (std::abs(d1 + d2 - segLen) < 1e-3) repeaters.push_back(rp);
            }

            // 绘制：节点 → 中继器 → 节点
            std::vector<QPointF> drawPts = {a};
            drawPts.insert(drawPts.end(), repeaters.begin(), repeaters.end());
            drawPts.push_back(b);

            for (size_t j = 1; j < drawPts.size(); ++j)
                painter.drawPath(createArc(drawPts[j-1], drawPts[j], 30.0));
        }
    }

    // ===== 绘制终端电阻和网桥 =====
    painter.setPen(QPen(Qt::blue, 2));
    painter.setBrush(Qt::NoBrush);

    int termIndex = 0;
    for (size_t si = 0; si < result_.segments.size(); ++si) {
        const auto &seg = result_.segments[si];

        // ① 第一个段起点终端电阻
        if (si == 0) {
            QPointF termStart = scalePoint(QPointF(result_.devices.terminator_positions[termIndex].first,
                                                   result_.devices.terminator_positions[termIndex].second),
                                           minX, minY, scale, margin);
            termStart = adjustDevicePosition(termStart, existingDevices, 30.0);
            existingDevices.push_back(termStart);

            QPointF segStart = scalePoint(nodes_[seg.node_ids[0]], minX, minY, scale, margin);
            painter.drawPath(createArc(segStart, termStart, 30.0));

            painter.setBrush(Qt::darkGreen);
            painter.drawRect(termStart.x()-3, termStart.y()-3, 6, 6);
            painter.setBrush(Qt::NoBrush);

            termIndex++;
        }

        // ② 中间段处理（网桥）
        if (si < result_.segments.size() - 1) {
            QPointF segEnd = scalePoint(nodes_[seg.node_ids.back()], minX, minY, scale, margin);
            QPointF bridgePos = scalePoint(QPointF(result_.devices.bridge_positions[si].first,
                                                   result_.devices.bridge_positions[si].second),
                                           minX, minY, scale, margin);
            bridgePos = adjustDevicePosition(bridgePos, existingDevices, 30.0);
            existingDevices.push_back(bridgePos);

            // 当前段终点 → 网桥
            painter.drawPath(createArc(segEnd, bridgePos, 30.0));

            // 当前段终点的终端电阻
            QPointF termEnd = scalePoint(QPointF(result_.devices.terminator_positions[termIndex].first,
                                                 result_.devices.terminator_positions[termIndex].second),
                                         minX, minY, scale, margin);
            termEnd = adjustDevicePosition(termEnd, existingDevices, 30.0);
            existingDevices.push_back(termEnd);
            painter.drawPath(createArc(bridgePos, termEnd, 30.0));
            painter.setBrush(Qt::darkGreen);
            painter.drawRect(termEnd.x()-3, termEnd.y()-3, 6, 6);
            painter.setBrush(Qt::NoBrush);
            termIndex++;

            // 网桥 → 下一段起点
            QPointF nextSegStart = scalePoint(nodes_[result_.segments[si+1].node_ids[0]], minX, minY, scale, margin);
            painter.drawPath(createArc(bridgePos, nextSegStart, 30.0));

            // 下一段起点终端电阻
            QPointF nextTerm = scalePoint(QPointF(result_.devices.terminator_positions[termIndex].first,
                                                  result_.devices.terminator_positions[termIndex].second),
                                          minX, minY, scale, margin);
            nextTerm = adjustDevicePosition(nextTerm, existingDevices, 30.0);
            existingDevices.push_back(nextTerm);
            painter.drawPath(createArc(bridgePos, nextTerm, 30.0));
            painter.setBrush(Qt::darkGreen);
            painter.drawRect(nextTerm.x()-3, nextTerm.y()-3, 6, 6);
            painter.setBrush(Qt::NoBrush);
            termIndex++;

            // 绘制网桥
            painter.setBrush(Qt::magenta);
            painter.drawRect(bridgePos.x()-4, bridgePos.y()-4, 8, 8);
            painter.setBrush(Qt::NoBrush);
        }
        // ③ 最后一段终点终端电阻
        else {
            QPointF segEnd = scalePoint(nodes_[seg.node_ids.back()], minX, minY, scale, margin);
            QPointF termEnd = scalePoint(QPointF(result_.devices.terminator_positions[termIndex].first,
                                                 result_.devices.terminator_positions[termIndex].second),
                                         minX, minY, scale, margin);
            termEnd = adjustDevicePosition(termEnd, existingDevices, 30.0);
            existingDevices.push_back(termEnd);
            painter.drawPath(createArc(segEnd, termEnd, 30.0));

            painter.setBrush(Qt::darkGreen);
            painter.drawRect(termEnd.x()-3, termEnd.y()-3, 6, 6);
            painter.setBrush(Qt::NoBrush);

            termIndex++;
        }
    }

    // ===== 绘制中继器 =====
    painter.setBrush(Qt::blue);
    for (auto &r : result_.devices.repeater_positions) {
        QPointF p = scalePoint(QPointF(r.first, r.second), minX, minY, scale, margin);
        p = adjustDevicePosition(p, existingDevices, 10.0);
        existingDevices.push_back(p);
        painter.drawEllipse(p.x()-4, p.y()-4, 8, 8);
    }

    // ===== 绘制节点 =====
    painter.setBrush(Qt::red);
    for (auto &pt : nodes_) {
        QPointF p = scalePoint(pt, minX, minY, scale, margin);
        painter.drawEllipse(p.x()-5, p.y()-5, 10, 10);
    }
}
