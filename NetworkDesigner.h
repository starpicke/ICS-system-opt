//NetworkDesigner.h
#pragma once
#include <vector>
#include <string>
#include <utility>
#include <random>

namespace IndustrialNet {

struct Node {
    int id = -1;
    double x = 0.0;
    double y = 0.0;
    double input_impedance_ohm = 1e6;
};

struct Segment {
    int id = -1;
    double start_pos_m = 0.0;
    double end_pos_m = 0.0;
    std::vector<int> node_ids;
};

struct DevicePlacement {
    std::vector<std::pair<double, double>> terminator_positions;
    std::vector<std::pair<double, double>> repeater_positions;

};

struct DesignResult {
    std::vector<Segment> segments;
    DevicePlacement devices;
    std::vector<std::pair<int, bool>> node_receive_ok;
    std::vector<std::string> logs;
    bool overall_ok = false;
    double total_path_length = 0.0;  // 总路径长度
};

struct DesignerParams {
    int max_nodes_per_segment = 30;
    double cable_atten_dB_per_100m = 2.0;
    double driver_peak_voltage_v = 1.2;
    double driver_source_impedance_ohm = 30.0;
    double required_min_receive_v = 0.9;
    double termination_impedance_ohm = 120.0;
    double RL_ohm = 40000.0;
    double Rw_factor = 0.0214;

    // Terminator布局控制
    double terminator_offset_along = 2.0;
    double terminator_offset_perp = 2.0;
    double terminator_min_distance = 1.0;

    // 新增：允许外部动态设置
    double max_segment_length_m = 40.0;
};

// 添加蚁群算法参数
struct ACOParams {
    int ant_count = 20;                    // 蚂蚁数量
    int max_iterations = 100;              // 最大迭代次数
    double alpha = 1.0;                    // 信息素重要性
    double beta = 8.0;                     // 启发式信息重要性
    double evaporation_rate = 0.5;         // 信息素蒸发率
    double q0 = 0.7;                       // 探索概率
    double initial_pheromone = 1.0;        // 初始信息素
};

class NetworkDesigner {
public:
    explicit NetworkDesigner(const DesignerParams& params = DesignerParams());

    // 设置蚁群算法参数
    void setACOParams(const ACOParams& aco_params);

    // ===== 外部接口 =====
    DesignResult designNetwork(const std::vector<Node>& nodes);
    bool generateSVG(const std::vector<Node>& nodes, const DesignResult& result, const std::string& filename);
    void printDesignReport(const std::vector<Node>& nodes, const DesignResult& result);




private:
    DesignerParams p_;
    ACOParams aco_params_;  // 蚁群算法参数

    // 距离矩阵计算方法
    std::vector<std::vector<double>> calculateDistanceMatrix(const std::vector<Node>& nodes) const {
        std::vector<std::vector<double>> distances(nodes.size(),
                                                   std::vector<double>(nodes.size(), 0.0));
        for (size_t i = 0; i < nodes.size(); ++i) {
            for (size_t j = i + 1; j < nodes.size(); ++j) {
                double dist = distance2D(nodes[i].x, nodes[i].y, nodes[j].x, nodes[j].y);
                distances[i][j] = dist;
                distances[j][i] = dist;
            }
        }
        return distances;
    }

    // 随机数生成器
    mutable std::mt19937 rng_;

    // 蚁群算法方法
    std::vector<Segment> partitionWithACO(const std::vector<Node>& nodes);
    double calculateSegmentQuality(const std::vector<Node>& nodes, const std::vector<int>& segment_nodes) const;
    double calculateHeuristic(const std::vector<Node>& nodes, int from, int to) const;
    int selectNextNode(const std::vector<Node>& nodes,
                       const std::vector<bool>& visited,
                       int current_node,
                       const std::vector<std::vector<double>>& pheromone) const;
    void updatePheromone(std::vector<std::vector<double>>& pheromone,
                         const std::vector<std::vector<Segment>>& solutions,
                         const std::vector<Node>& nodes) const;

    double calculateTotalPathLength(const std::vector<Node>& nodes,
                                    const std::vector<Segment>& segments) const;


    std::vector<int> findShortestPathACO(const std::vector<Node>& nodes);
    std::vector<Segment> splitPathIntoSegments(const std::vector<Node>& nodes,
                                               const std::vector<int>& path);
    int selectNextNodeACO(const std::vector<Node>& nodes,
                          const std::vector<bool>& visited,
                          int current_node,
                          const std::vector<std::vector<double>>& pheromone,
                          const std::vector<std::vector<double>>& distances) const;
    double calculatePathDistance(const std::vector<int>& path,
                                 const std::vector<std::vector<double>>& distances) const;
    void updatePheromoneACO(std::vector<std::vector<double>>& pheromone,
                            const std::vector<std::vector<int>>& solutions,
                            const std::vector<double>& distances) const;

    // 分段
    //std::vector<Segment> partitionIntoSegments(const std::vector<Node>& nodes);
    void refineSegmentsByVin(const std::vector<Node>& nodes, std::vector<Segment>& segs);
    DevicePlacement planDevices(const std::vector<Node>& nodes, const std::vector<Segment>& segs);
    std::vector<std::pair<int, bool>> checkReceiveLevels(const std::vector<Node>& nodes, const std::vector<Segment>& segs);

    // ===== 工具函数 =====
    double estimateReceivedVoltage(double Vout, double segment_length_m, int n_nodes_on_segment) const;
    double segmentLengthAlongNodes(const std::vector<Node>& nodes, const Segment& seg) const;
    double distance2D(double x1, double y1, double x2, double y2) const;
    std::pair<double,double> adjustTerminatorPosition(
        double x, double y,
        const std::vector<std::pair<double,double>>& existing_devices,
        const std::vector<Node>& nodes) const;
    };

} // namespace IndustrialNet
