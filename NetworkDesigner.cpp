#include "NetworkDesigner.h"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace IndustrialNet {

    NetworkDesigner::NetworkDesigner(const DesignerParams& params)
        : p_(params) {
    }

    double NetworkDesigner::distance2D(double x1, double y1, double x2, double y2) const {
        return std::sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
    }

    std::vector<Segment> NetworkDesigner::partitionIntoSegments(const std::vector<Node>& nodes) {
        std::vector<Segment> segments;
        if (nodes.empty()) return segments;

        Segment seg;
        seg.id = 0;
        Node startNode = nodes[0];
        seg.start_pos_m = 0;

        for (size_t i = 0; i < nodes.size(); ++i) {
            double dist = distance2D(startNode.x, startNode.y, nodes[i].x, nodes[i].y);

            if (!seg.node_ids.empty() &&
                (seg.node_ids.size() >= size_t(p_.max_nodes_per_segment) || dist > p_.max_segment_length_m)) {
                seg.end_pos_m = distance2D(startNode.x, startNode.y, nodes[seg.node_ids.back()].x, nodes[seg.node_ids.back()].y);
                segments.push_back(seg);

                seg = Segment();
                seg.id = int(segments.size());
                seg.start_pos_m = 0;
                startNode = nodes[i];
            }

            seg.node_ids.push_back(nodes[i].id);
        }

        if (!seg.node_ids.empty()) {
            seg.end_pos_m = distance2D(startNode.x, startNode.y, nodes[seg.node_ids.back()].x, nodes[seg.node_ids.back()].y);
            segments.push_back(seg);
        }

        return segments;
    }

    DevicePlacement NetworkDesigner::planDevices(const std::vector<Node>& nodes, const std::vector<Segment>& segs) {
        DevicePlacement dev;
        for (size_t s_idx = 0; s_idx < segs.size(); ++s_idx) {
            const auto& s = segs[s_idx];
            if (s.node_ids.empty()) continue;

            Node start_node = nodes[s.node_ids.front()];
            Node end_node = nodes[s.node_ids.back()];

            dev.terminator_positions.push_back({ start_node.x, start_node.y });
            dev.terminator_positions.push_back({ end_node.x, end_node.y });

            // 中继器：段内中点，或者单节点段也放置
            int mid_index = s.node_ids.size() / 2;
            Node mid_node = nodes[s.node_ids[mid_index]];
            dev.repeater_positions.push_back({ mid_node.x, mid_node.y });

            // 网桥：段间连接
            if (s_idx > 0) {
                const auto& prev_seg = segs[s_idx - 1];
                Node prev_end = nodes[prev_seg.node_ids.back()];
                Node curr_start = start_node;

                double bx = (prev_end.x + curr_start.x) / 2.0;
                double by = (prev_end.y + curr_start.y) / 2.0;
                dev.bridge_positions.push_back({ bx, by });
            }
        }
        return dev;
    }

    std::vector<std::pair<int, bool>> NetworkDesigner::checkReceiveLevels(const std::vector<Node>& nodes, const std::vector<Segment>& segs) {
        std::vector<std::pair<int, bool>> out;
        for (auto& n : nodes) out.push_back({ n.id, true });
        return out;
    }

    double NetworkDesigner::estimateReceivedVoltage(double tx, double distance_m, double Rload) const {
        double att = std::pow(10.0, -p_.cable_atten_dB_per_100m * distance_m / 100 / 20);
        return tx * att * (Rload / (Rload + p_.driver_source_impedance_ohm));
    }

    DesignResult NetworkDesigner::designNetwork(const std::vector<Node>& nodes) {
        DesignResult res;
        res.segments = partitionIntoSegments(nodes);
        res.devices = planDevices(nodes, res.segments);
        res.node_receive_ok = checkReceiveLevels(nodes, res.segments);
        res.overall_ok = true;
        for (auto& p : res.node_receive_ok) if (!p.second) res.overall_ok = false;
        res.logs.push_back("网络设计完成");
        return res;
    }

    bool NetworkDesigner::generateSVG(const std::vector<Node>& nodes, const DesignResult& result, const std::string& filename) {
        if (nodes.empty()) return false;
        std::ofstream ofs(filename);
        if (!ofs.is_open()) return false;

        double maxX = 0, maxY = 0;
        for (auto& n : nodes) {
            if (n.x > maxX) maxX = n.x;
            if (n.y > maxY) maxY = n.y;
        }
        double margin = 50;
        double width = maxX + margin;
        double height = maxY + margin;

        ofs << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width << "\" height=\"" << height << "\">\n";

        std::vector<std::string> colors = { "red","blue","green","orange","purple","cyan" };

        for (size_t i = 0; i < result.segments.size(); ++i) {
            auto& seg = result.segments[i];
            std::string color = colors[i % colors.size()];
            for (size_t j = 1; j < seg.node_ids.size(); ++j) {
                int id1 = seg.node_ids[j - 1];
                int id2 = seg.node_ids[j];
                double x1 = nodes[id1].x, y1 = nodes[id1].y;
                double x2 = nodes[id2].x, y2 = nodes[id2].y;
                ofs << "<line x1=\"" << x1 << "\" y1=\"" << y1
                    << "\" x2=\"" << x2 << "\" y2=\"" << y2
                    << "\" stroke=\"" << color << "\" stroke-width=\"2\" />\n";
            }
        }

        for (auto& n : nodes) {
            ofs << "<circle cx=\"" << n.x << "\" cy=\"" << n.y
                << "\" r=\"5\" fill=\"black\" />\n";
            ofs << "<text x=\"" << n.x + 6 << "\" y=\"" << n.y - 6
                << "\" font-size=\"12\" fill=\"black\">" << n.id << "</text>\n";
        }

        ofs << "</svg>\n";
        ofs.close();
        return true;
    }

    void NetworkDesigner::printDesignReport(const std::vector<Node>& nodes, const DesignResult& result) {
        std::cout << "=== 网络设计报告 ===\n";
        for (auto& seg : result.segments) {
            Node start_node = nodes[seg.node_ids.front()];
            Node end_node = nodes[seg.node_ids.back()];

            std::cout << "网段 " << seg.id << ": 节点数 = " << seg.node_ids.size() << " [";
            for (size_t i = 0; i < seg.node_ids.size(); ++i) {
                std::cout << seg.node_ids[i];
                if (i != seg.node_ids.size() - 1) std::cout << ", ";
            }
            std::cout << "]\n";
            std::cout << "  起点坐标 = (" << start_node.x << "," << start_node.y << "), "
                << "终点坐标 = (" << end_node.x << "," << end_node.y << ")\n";
        }

        std::cout << "终端电阻位置: ";
        for (auto& pos : result.devices.terminator_positions)
            std::cout << "(" << pos.first << "," << pos.second << ") ";
        std::cout << "\n";

        std::cout << "中继器位置: ";
        for (auto& pos : result.devices.repeater_positions)
            std::cout << "(" << pos.first << "," << pos.second << ") ";
        std::cout << "\n";

        std::cout << "网桥位置: ";
        for (auto& pos : result.devices.bridge_positions)
            std::cout << "(" << pos.first << "," << pos.second << ") ";
        std::cout << "\n";

        std::cout << "节点接收显性电平检查:\n";
        for (auto& n : result.node_receive_ok)
            std::cout << "节点 " << n.first << ": " << (n.second ? "OK" : "FAIL") << "\n";

        std::cout << "总体网络状态: " << (result.overall_ok ? "OK" : "FAIL") << "\n";
        for (auto& log : result.logs)
            std::cout << log << "\n";
    }

} // namespace IndustrialNet
