#include "NetworkDesigner.h"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace IndustrialNet {

NetworkDesigner::NetworkDesigner(const DesignerParams& params)
    : p_(params) {}

double NetworkDesigner::distance2D(double x1, double y1, double x2, double y2) const {
    return std::sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
}

double NetworkDesigner::segmentLengthAlongNodes(const std::vector<Node>& nodes, const Segment& seg) const {
    if (seg.node_ids.size() < 2) return 0.0;
    double sum = 0.0;
    for (size_t i = 1; i < seg.node_ids.size(); ++i) {
        const Node& a = nodes[seg.node_ids[i-1]];
        const Node& b = nodes[seg.node_ids[i]];
        sum += distance2D(a.x, a.y, b.x, b.y);
    }
    return sum;
}

double NetworkDesigner::estimateReceivedVoltage(double Vout, double segment_length_m, int n_nodes_on_segment) const {
    double Rw = p_.Rw_factor * segment_length_m;
    double Rt = p_.termination_impedance_ohm;
    double RL = p_.RL_ohm;
    int n = std::max(1, n_nodes_on_segment);

    double inner = (1.0 / Rt) - (double)(n - 1) / RL;
    double denom = 1.0 + 2.0 * Rw * inner;
    if (denom <= 0.0) return 0.0;

    return Vout / denom;
}

std::vector<Segment> NetworkDesigner::partitionIntoSegments(const std::vector<Node>& nodes) {
    std::vector<Segment> segments;
    if (nodes.empty()) return segments;

    Segment seg;
    seg.id = 0;

    double accum_len = 0.0;             // 当前段的累计长度（沿节点）
    int nodes_in_seg = 0;               // 当前段节点数
    // 辅助：记录上一个节点坐标用于增量计算
    double last_x = 0.0, last_y = 0.0;
    bool has_last = false;

    for (size_t i = 0; i < nodes.size(); ++i) {
        const Node& cur = nodes[i];

        if (!has_last) {
            // 段刚开始，加入第一个节点
            seg.node_ids.push_back(cur.id);
            nodes_in_seg = 1;
            accum_len = 0.0;
            last_x = cur.x;
            last_y = cur.y;
            has_last = true;
            continue;
        }

        // 计算从上一个节点到当前节点的边长
        double edge = distance2D(last_x, last_y, cur.x, cur.y);

        // 如果加入当前节点会违反长度或节点数限制，则先把当前段收尾并开启新段
        bool exceed_length = (accum_len + edge) > p_.max_segment_length_m;
        bool exceed_nodes = (nodes_in_seg + 1) > p_.max_nodes_per_segment;

        if (exceed_length || exceed_nodes) {
            // 结束当前段
            seg.start_pos_m = 0.0;
            seg.end_pos_m = segmentLengthAlongNodes(nodes, seg); // 保持与现有逻辑一致
            segments.push_back(seg);

            // 新段初始化，从当前节点开始（不把边加入上段）
            seg = Segment();
            seg.id = int(segments.size());
            seg.node_ids.clear();
            seg.node_ids.push_back(cur.id);
            nodes_in_seg = 1;
            accum_len = 0.0;
            last_x = cur.x;
            last_y = cur.y;
            // has_last remains true
        } else {
            // 可以把当前节点加入本段
            seg.node_ids.push_back(cur.id);
            accum_len += edge;
            nodes_in_seg += 1;
            last_x = cur.x;
            last_y = cur.y;
        }
    }

    // 推入最后一个未保存的段
    if (!seg.node_ids.empty()) {
        seg.start_pos_m = 0.0;
        seg.end_pos_m = segmentLengthAlongNodes(nodes, seg);
        segments.push_back(seg);
    }

    return segments;
}


void NetworkDesigner::refineSegmentsByVin(const std::vector<Node>& nodes, std::vector<Segment>& segs) {
    bool changed = false;
    std::vector<Segment> newSegs;

    for (auto& s : segs) {
        double segLen = segmentLengthAlongNodes(nodes, s);
        int n = int(s.node_ids.size());
        double Vin = estimateReceivedVoltage(p_.driver_peak_voltage_v, segLen, n);

        if (Vin >= p_.required_min_receive_v || n <= 1) {
            newSegs.push_back(s);
            continue;
        }

        changed = true;
        size_t mid = n / 2;
        Segment a; a.id = int(newSegs.size());
        a.node_ids.insert(a.node_ids.end(), s.node_ids.begin(), s.node_ids.begin()+mid);
        a.end_pos_m = segmentLengthAlongNodes(nodes, a);

        Segment b; b.id = int(newSegs.size())+1;
        b.node_ids.insert(b.node_ids.end(), s.node_ids.begin()+mid, s.node_ids.end());
        b.end_pos_m = segmentLengthAlongNodes(nodes, b);

        if (a.node_ids.empty() || b.node_ids.empty()) newSegs.push_back(s);
        else { newSegs.push_back(a); newSegs.push_back(b); }
    }

    if (changed) refineSegmentsByVin(nodes, newSegs);
    segs = std::move(newSegs);
}

// 通用版设备布置
// ===== planDevices() 完整版本 =====
DevicePlacement NetworkDesigner::planDevices(const std::vector<Node>& nodes, const std::vector<Segment>& segs) {
    DevicePlacement dev;

    for (size_t i = 0; i < segs.size(); ++i) {
        const Segment& s = segs[i];
        if (s.node_ids.empty()) continue;

        const Node& start = nodes[s.node_ids.front()];
        const Node& end   = nodes[s.node_ids.back()];

        // ===== 起点终端电阻 =====
        double sx = start.x;
        double sy = start.y;
        if (s.node_ids.size() >= 2) {
            const Node& next = nodes[s.node_ids[1]];
            double dx = sx - next.x;
            double dy = sy - next.y;
            double len = std::sqrt(dx*dx + dy*dy);
            if (len > 0.0) {
                dx /= len; dy /= len;
                sx += dx * p_.terminator_offset_along;
                sy += dy * p_.terminator_offset_along;
            }
        }
        dev.terminator_positions.push_back(adjustTerminatorPosition(sx, sy, dev.terminator_positions, nodes));

        // ===== 终点终端电阻 =====
        double ex = end.x;
        double ey = end.y;
        if (s.node_ids.size() >= 2) {
            const Node& prev = nodes[s.node_ids[s.node_ids.size() - 2]];
            double dx = ex - prev.x;
            double dy = ey - prev.y;
            double len = std::sqrt(dx*dx + dy*dy);
            if (len > 0.0) {
                dx /= len; dy /= len;
                ex += dx * p_.terminator_offset_along;
                ey += dy * p_.terminator_offset_along;
            }
        }
        dev.terminator_positions.push_back(adjustTerminatorPosition(ex, ey, dev.terminator_positions, nodes));

        // ===== 中继器逻辑 =====
        double segLen = segmentLengthAlongNodes(nodes, s);
        if (segLen > p_.max_segment_length_m || s.node_ids.size() > size_t(p_.max_nodes_per_segment)) {
            if (s.node_ids.size() >= 2) {
                size_t midIndex = s.node_ids.size() / 2;
                const Node& a = nodes[s.node_ids[midIndex - 1]];
                const Node& b = nodes[s.node_ids[midIndex]];
                double rx = (a.x + b.x) / 2.0;
                double ry = (a.y + b.y) / 2.0;
                dev.repeater_positions.push_back({ rx, ry });
            }
        }
    }

    // ===== 网桥逻辑 =====
    // 每两个段之间一个网桥
    for (size_t i = 0; i + 1 < segs.size(); ++i) {
        const Segment& cur = segs[i];
        const Segment& nxt = segs[i + 1];
        const Node& end_cur    = nodes[cur.node_ids.back()];
        const Node& start_next = nodes[nxt.node_ids.front()];
        double bx = (end_cur.x + start_next.x) / 2.0;
        double by = (end_cur.y + start_next.y) / 2.0;
        dev.bridge_positions.push_back({ bx, by });
    }

    return dev;
}

// ===== adjustTerminatorPosition() 实现 =====
std::pair<double,double> NetworkDesigner::adjustTerminatorPosition(
    double x, double y,
    const std::vector<std::pair<double,double>>& existing_devices,
    const std::vector<Node>& nodes) const
{
    double min_dist = p_.terminator_min_distance;
    double max_offset = p_.terminator_offset_along * 5; // 最大偏移限制
    double adjusted_x = x;
    double adjusted_y = y;

    // 方向向量，这里默认沿 x/y 偏移，可以改成传入向量
    double step = 0.5; // 每次微调距离
    double offset = 0.0;

    bool conflict = true;
    while(conflict && offset < max_offset) {
        conflict = false;

        // 检查与已有终端电阻
        for(auto& dev : existing_devices) {
            double dx = adjusted_x - dev.first;
            double dy = adjusted_y - dev.second;
            if(std::sqrt(dx*dx + dy*dy) < min_dist) {
                conflict = true;
                break;
            }
        }

        // 检查与节点
        for(auto& n : nodes) {
            double dx = adjusted_x - n.x;
            double dy = adjusted_y - n.y;
            if(std::sqrt(dx*dx + dy*dy) < min_dist) {
                conflict = true;
                break;
            }
        }

        if(conflict) {
            // 沿初始方向反向微调
            adjusted_x += step;
            adjusted_y += step;
            offset += step;
        }
    }

    return {adjusted_x, adjusted_y};
}

std::vector<std::pair<int,bool>> NetworkDesigner::checkReceiveLevels(const std::vector<Node>& nodes, const std::vector<Segment>& segs) {
    std::vector<std::pair<int,bool>> out;
    out.reserve(nodes.size());
    for (auto& n : nodes) out.push_back({ n.id,true });

    for (auto& s : segs) {
        double segLen = segmentLengthAlongNodes(nodes, s);
        int n = int(s.node_ids.size());
        bool ok = estimateReceivedVoltage(p_.driver_peak_voltage_v, segLen, n) >= p_.required_min_receive_v;
        for (int nid : s.node_ids) out[nid].second = ok;
    }

    return out;
}

DesignResult NetworkDesigner::designNetwork(const std::vector<Node>& nodes) {
    DesignResult res;

    res.segments = partitionIntoSegments(nodes);
    refineSegmentsByVin(nodes, res.segments);
    res.devices = planDevices(nodes, res.segments);
    res.node_receive_ok = checkReceiveLevels(nodes, res.segments);

    res.overall_ok = true;
    for (auto& p : res.node_receive_ok) if (!p.second) res.overall_ok = false;

    std::ostringstream ss;
    ss << "网络设计完成. 段数=" << res.segments.size();
    res.logs.push_back(ss.str());

    return res;
}

// SVG生成和打印报告保持不变
#include <fstream>
bool NetworkDesigner::generateSVG(const std::vector<Node>& nodes, const DesignResult& result, const std::string& filename) {
    if (nodes.empty()) return false;
    std::ofstream ofs(filename);
    if (!ofs.is_open()) return false;

    double maxX = 0, maxY = 0;
    for (auto& n : nodes) { if(n.x>maxX) maxX=n.x; if(n.y>maxY) maxY=n.y; }
    double margin = 50;
    ofs << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << maxX+margin << "\" height=\"" << maxY+margin << "\">\n";

    std::vector<std::string> colors = { "red","blue","green","orange","purple","cyan","brown","magenta" };
    for (size_t si=0; si<result.segments.size(); ++si) {
        const Segment& s = result.segments[si];
        std::string color = colors[si % colors.size()];
        for (size_t j=1;j<s.node_ids.size();++j){
            int id1=s.node_ids[j-1], id2=s.node_ids[j];
            ofs << "<line x1=\"" << nodes[id1].x << "\" y1=\"" << nodes[id1].y
                << "\" x2=\"" << nodes[id2].x << "\" y2=\"" << nodes[id2].y
                << "\" stroke=\"" << color << "\" stroke-width=\"2\" />\n";
        }
    }

    for (auto& n : nodes){
        ofs << "<circle cx=\"" << n.x << "\" cy=\"" << n.y << "\" r=\"5\" fill=\"black\" />\n";
        ofs << "<text x=\"" << n.x+6 << "\" y=\"" << n.y-6 << "\" font-size=\"12\" fill=\"black\">" << n.id << "</text>\n";
    }

    for (auto& r : result.devices.repeater_positions)
        ofs << "<rect x=\"" << r.first-4 << "\" y=\"" << r.second-4 << "\" width=\"8\" height=\"8\" fill=\"blue\" />\n";

    for (auto& b : result.devices.bridge_positions){
        double x=b.first,y=b.second;
        ofs << "<polygon points=\"" << x << "," << (y-5) << " " << (x+5) << "," << y
            << " " << x << "," << (y+5) << " " << (x-5) << "," << y << "\" fill=\"purple\" />\n";
    }

    for (auto& t : result.devices.terminator_positions)
        ofs << "<rect x=\"" << t.first-3 << "\" y=\"" << t.second-3 << "\" width=\"6\" height=\"6\" fill=\"red\" />\n";

    ofs << "</svg>\n";
    ofs.close();
    return true;
}

void NetworkDesigner::printDesignReport(const std::vector<Node>& nodes, const DesignResult& result){
    std::cout << "=== 网络设计报告 ===\n";
    for (auto& seg : result.segments) {
        Node start_node = nodes[seg.node_ids.front()];
        Node end_node = nodes[seg.node_ids.back()];
        std::cout << "网段 " << seg.id << ": 节点数=" << seg.node_ids.size() << " [";
        for (size_t i=0;i<seg.node_ids.size();++i){
            std::cout << seg.node_ids[i];
            if(i!=seg.node_ids.size()-1) std::cout << ", ";
        }
        std::cout << "]\n";
        std::cout << "  起点坐标=(" << start_node.x << "," << start_node.y << "), "
            << "终点坐标=(" << end_node.x << "," << end_node.y << ")\n";
        double segLen = segmentLengthAlongNodes(nodes, seg);
        double Vin = estimateReceivedVoltage(p_.driver_peak_voltage_v, segLen, (int)seg.node_ids.size());
        std::cout << "  段长度=" << segLen << ", 估算 Vin=" << Vin << " V\n";
    }

    std::cout << "终端电阻位置: ";
    for(auto& pos : result.devices.terminator_positions)
        std::cout << "(" << pos.first << "," << pos.second << ") ";
    std::cout << "\n";

    std::cout << "中继器位置: ";
    for(auto& pos : result.devices.repeater_positions)
        std::cout << "(" << pos.first << "," << pos.second << ") ";
    std::cout << "\n";

    std::cout << "网桥位置: ";
    for(auto& pos : result.devices.bridge_positions)
        std::cout << "(" << pos.first << "," << pos.second << ") ";
    std::cout << "\n";

    std::cout << "节点接收显性电平检查:\n";
    for(auto& n : result.node_receive_ok)
        std::cout << "节点 " << n.first << ": " << (n.second ? "OK" : "FAIL") << "\n";

    std::cout << "总体网络状态: " << (result.overall_ok ? "OK" : "FAIL") << "\n";
    for(auto& log : result.logs) std::cout << log << "\n";
}

} // namespace IndustrialNet
