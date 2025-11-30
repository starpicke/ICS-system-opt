//NetworkDesigner.cpp
#include "NetworkDesigner.h"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <random>

namespace IndustrialNet {

NetworkDesigner::NetworkDesigner(const DesignerParams& params)
    : p_(params), rng_(std::random_device{}()) {}  // 初始化随机数生成器

void NetworkDesigner::setACOParams(const ACOParams& aco_params) {
    aco_params_ = aco_params;
}

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


std::vector<Segment> NetworkDesigner::partitionWithACO(const std::vector<Node>& nodes) {
    if (nodes.empty()) return {};

    // 第一步：找到最短路径（不分段）
    auto shortest_path = findShortestPathACO(nodes);

    // 第二步：根据约束条件分段
    return splitPathIntoSegments(nodes, shortest_path);
}

// 新增：找到最短路径（不分段）
std::vector<int> NetworkDesigner::findShortestPathACO(const std::vector<Node>& nodes) {
    auto distances = calculateDistanceMatrix(nodes);
    int n_nodes = nodes.size();

    std::vector<std::vector<double>> pheromone(n_nodes,
                                               std::vector<double>(n_nodes, aco_params_.initial_pheromone));

    std::vector<int> best_path;
    double best_distance = 1e9;

    for (int iter = 0; iter < aco_params_.max_iterations; ++iter) {
        std::vector<std::vector<int>> all_paths;
        std::vector<double> all_distances;

        for (int ant = 0; ant < aco_params_.ant_count; ++ant) {
            // 随机选择起点
            std::uniform_int_distribution<int> start_dist(0, n_nodes - 1);
            int start_node = start_dist(rng_);

            std::vector<int> path = {start_node};
            std::vector<bool> visited(n_nodes, false);
            visited[start_node] = true;
            int current_node = start_node;

            // 构建完整路径
            while (path.size() < (size_t)n_nodes) {
                std::vector<int> unvisited;
                for (int i = 0; i < n_nodes; ++i) {
                    if (!visited[i]) unvisited.push_back(i);
                }
                if (unvisited.empty()) break;

                int next_node = selectNextNodeACO(nodes, visited, current_node, pheromone, distances);
                if (next_node == -1) break;

                path.push_back(next_node);
                visited[next_node] = true;
                current_node = next_node;
            }

            // 计算路径距离
            double distance = calculatePathDistance(path, distances);
            all_paths.push_back(path);
            all_distances.push_back(distance);

            if (distance < best_distance) {
                best_distance = distance;
                best_path = path;
            }
        }

        // 更新信息素
        updatePheromoneACO(pheromone, all_paths, all_distances);
    }

    return best_path;
}

// 根据约束条件分段
std::vector<Segment> NetworkDesigner::splitPathIntoSegments(const std::vector<Node>& nodes,
                                                            const std::vector<int>& path) {
    std::vector<Segment> segments;
    if (path.empty()) return segments;

    // 设置余量系数（1.2-1.5倍）
    double length_margin = 0.8;  // 1.3倍余量
    double max_length_with_margin = p_.max_segment_length_m * length_margin;

    Segment current_segment;
    current_segment.id = 0;
    current_segment.node_ids.push_back(path[0]);

    for (size_t i = 1; i < path.size(); ++i) {
        // 检查添加节点是否违反约束（使用带余量的长度）
        std::vector<int> temp_segment = current_segment.node_ids;
        temp_segment.push_back(path[i]);

        Segment temp_seg;
        temp_seg.node_ids = temp_segment;
        double seg_length = segmentLengthAlongNodes(nodes, temp_seg);

        // 使用带余量的长度检查
        bool exceed_length = seg_length > max_length_with_margin;
        bool exceed_nodes = temp_segment.size() > (size_t)p_.max_nodes_per_segment;

        if (exceed_length || exceed_nodes) {
            // 结束当前段
            if (current_segment.node_ids.size() >= 2) {
                current_segment.start_pos_m = 0.0;
                current_segment.end_pos_m = segmentLengthAlongNodes(nodes, current_segment);
                segments.push_back(current_segment);
            }
            // 开始新段
            current_segment = Segment();
            current_segment.id = segments.size();
            current_segment.node_ids.push_back(path[i]);  // 新段从当前节点开始
        } else {
            current_segment.node_ids.push_back(path[i]);
        }
    }

    // 添加最后一个段
    if (current_segment.node_ids.size() >= 2) {
        current_segment.start_pos_m = 0.0;
        current_segment.end_pos_m = segmentLengthAlongNodes(nodes, current_segment);
        segments.push_back(current_segment);
    }

    return segments;
}
// 修改现有的 selectNextNode 方法（重命名）
int NetworkDesigner::selectNextNodeACO(const std::vector<Node>& nodes,
                                       const std::vector<bool>& visited,
                                       int current_node,
                                       const std::vector<std::vector<double>>& pheromone,
                                       const std::vector<std::vector<double>>& distances) const {
    std::vector<int> candidates;
    std::vector<double> probabilities;
    double total = 0.0;

    for (size_t i = 0; i < nodes.size(); ++i) {
        if (!visited[i]) {
            candidates.push_back(i);
            double tau = pheromone[current_node][i];
            double eta = 1.0 / (distances[current_node][i] + 1e-6);
            double prob = std::pow(tau, aco_params_.alpha) * std::pow(eta, aco_params_.beta);
            probabilities.push_back(prob);
            total += prob;
        }
    }

    if (candidates.empty()) return -1;

    if (total > 0) {
        for (auto& prob : probabilities) {
            prob /= total;
        }

        std::uniform_real_distribution<double> dist(0.0, 1.0);
        double r = dist(rng_);
        double sum = 0.0;

        for (size_t i = 0; i < probabilities.size(); ++i) {
            sum += probabilities[i];
            if (r <= sum) {
                return candidates[i];
            }
        }
    }

    return candidates[0];
}

// 新增：计算路径距离
double NetworkDesigner::calculatePathDistance(const std::vector<int>& path,
                                              const std::vector<std::vector<double>>& distances) const {
    double total_distance = 0.0;
    for (size_t i = 1; i < path.size(); ++i) {
        total_distance += distances[path[i-1]][path[i]];
    }
    return total_distance;
}

// 修改现有的 updatePheromone 方法
void NetworkDesigner::updatePheromoneACO(std::vector<std::vector<double>>& pheromone,
                                         const std::vector<std::vector<int>>& solutions,
                                         const std::vector<double>& distances) const {
    // 信息素蒸发
    for (auto& row : pheromone) {
        for (auto& value : row) {
            value *= (1.0 - aco_params_.evaporation_rate);
        }
    }

    // 信息素增强
    for (size_t i = 0; i < solutions.size(); ++i) {
        const auto& path = solutions[i];
        double distance = distances[i];
        double pheromone_to_add = aco_params_.initial_pheromone / distance;

        for (size_t j = 1; j < path.size(); ++j) {
            int from = path[j-1];
            int to = path[j];
            pheromone[from][to] += pheromone_to_add;
            pheromone[to][from] += pheromone_to_add;
        }
    }
}

//启发函数
double NetworkDesigner::calculateHeuristic(const std::vector<Node>& nodes, int from, int to) const {
    double dist = distance2D(nodes[from].x, nodes[from].y, nodes[to].x, nodes[to].y);

    // 相邻节点有很高的优先级
    if (abs(from - to) == 1) {
        return 10.0;
    }

    return 1.0 / (dist + 1e-6);
}


//质量评估
double NetworkDesigner::calculateSegmentQuality(const std::vector<Node>& nodes, const std::vector<int>& segment_nodes) const {
    if (segment_nodes.size() < 2) return -1000.0;

    double length = segmentLengthAlongNodes(nodes, Segment{0, 0.0, 0.0, segment_nodes});
    double voltage = estimateReceivedVoltage(p_.driver_peak_voltage_v, length, (int)segment_nodes.size());

    // 主要优化目标：最小化路径长度和段数
    double length_penalty = length * 0.1;  // 长度惩罚
    double segment_penalty = 100.0;        // 每增加一个段的惩罚

    // 电压约束（适当放宽）
    double voltage_score = (voltage >= p_.required_min_receive_v * 0.8) ? 0.0 : -500.0;

    // 连续性奖励（重要！）
    double continuity_score = 0.0;
    for (size_t i = 1; i < segment_nodes.size(); ++i) {
        if (abs(segment_nodes[i] - segment_nodes[i-1]) == 1) {
            continuity_score += 50.0; // 连续节点重奖
        } else {
            continuity_score -= 100.0; // 非连续节点重罚
        }
    }

    return -length_penalty - segment_penalty + voltage_score + continuity_score;
}


int NetworkDesigner::selectNextNode(const std::vector<Node>& nodes,
                                    const std::vector<bool>& visited,
                                    int current_node,
                                    const std::vector<std::vector<double>>& pheromone) const {
    std::vector<int> candidates;
    std::vector<double> probabilities;
    double total = 0.0;

    // 收集所有候选节点
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (!visited[i] && i != current_node) {
            candidates.push_back(i);
            double tau = pheromone[current_node][i];
            double eta = calculateHeuristic(nodes, current_node, i);
            double prob = std::pow(tau, aco_params_.alpha) * std::pow(eta, aco_params_.beta);
            probabilities.push_back(prob);
            total += prob;
        }
    }

    if (candidates.empty()) return -1;

    // 归一化概率
    for (auto& prob : probabilities) {
        prob /= total;
    }

    // 轮盘赌选择
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double r = dist(rng_);
    double sum = 0.0;

    for (size_t i = 0; i < probabilities.size(); ++i) {
        sum += probabilities[i];
        if (r <= sum) {
            return candidates[i];
        }
    }

    return candidates.back();
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

    // 遍历段，跳过空段
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
    }

    // ===== 中继器逻辑（每两个段之间一个中继器） =====
    for (size_t k = 0; k + 1 < segs.size(); ++k) {
        const Segment& cur = segs[k];
        const Segment& nxt = segs[k + 1];
        if (cur.node_ids.empty() || nxt.node_ids.empty()) continue;

        const Node& end_cur    = nodes[cur.node_ids.back()];
        const Node& start_next = nodes[nxt.node_ids.front()];
        double rx = (end_cur.x + start_next.x) / 2.0;
        double ry = (end_cur.y + start_next.y) / 2.0;

        dev.repeater_positions.push_back({ rx, ry });
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

    // 使用蚁群算法替换原有的分段算法
    res.segments = partitionWithACO(nodes);

    // 计算总路径长度
    res.total_path_length = calculateTotalPathLength(nodes, res.segments);

    refineSegmentsByVin(nodes, res.segments);
    res.devices = planDevices(nodes, res.segments);
    res.node_receive_ok = checkReceiveLevels(nodes, res.segments);

    res.overall_ok = true;
    for (auto& p : res.node_receive_ok) if (!p.second) res.overall_ok = false;

    return res;
}

// 计算总路径长度
double NetworkDesigner::calculateTotalPathLength(const std::vector<Node>& nodes,
                                                 const std::vector<Segment>& segments) const {
    double total_length = 0.0;

    // 方法1：直接计算所有段的长度之和
    for (const auto& seg : segments) {
        total_length += segmentLengthAlongNodes(nodes, seg);
    }

    return total_length;
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



    for (auto& b : result.devices.repeater_positions){
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



    std::cout << "节点接收显性电平检查:\n";
    for(auto& n : result.node_receive_ok)
        std::cout << "节点 " << n.first << ": " << (n.second ? "OK" : "FAIL") << "\n";

    std::cout << "总体网络状态: " << (result.overall_ok ? "OK" : "FAIL") << "\n";
    for(auto& log : result.logs) std::cout << log << "\n";
}

} // namespace IndustrialNet
