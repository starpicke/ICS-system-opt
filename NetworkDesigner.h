#pragma once
#include <vector>
#include <string>
#include <utility>

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
    std::vector<std::pair<double, double>> bridge_positions;
};

struct DesignResult {
    std::vector<Segment> segments;
    DevicePlacement devices;
    std::vector<std::pair<int, bool>> node_receive_ok; // per node id, pass/fail
    std::vector<std::string> logs;
    bool overall_ok = false;
};

struct DesignerParams {
    double max_segment_length_m = 40.0;
    int max_nodes_per_segment = 30;
    double cable_atten_dB_per_100m = 2.0;
    double driver_peak_voltage_v = 1.2;
    double driver_source_impedance_ohm = 30.0;
    double required_min_receive_v = 0.9;
    double termination_impedance_ohm = 120.0;
    double RL_ohm = 40000.0;
    double Rw_factor = 0.0214;
};

class NetworkDesigner {
public:
    explicit NetworkDesigner(const DesignerParams& params = DesignerParams());

    DesignResult designNetwork(const std::vector<Node>& nodes);
    bool generateSVG(const std::vector<Node>& nodes, const DesignResult& result, const std::string& filename);
    void printDesignReport(const std::vector<Node>& nodes, const DesignResult& result);

private:
    DesignerParams p_;

    std::vector<Segment> partitionIntoSegments(const std::vector<Node>& nodes);
    void refineSegmentsByVin(const std::vector<Node>& nodes, std::vector<Segment>& segs);

    DevicePlacement planDevices(const std::vector<Node>& nodes, const std::vector<Segment>& segs);
    std::vector<std::pair<int, bool>> checkReceiveLevels(const std::vector<Node>& nodes, const std::vector<Segment>& segs);

    double estimateReceivedVoltage(double Vout, double segment_length_m, int n_nodes_on_segment) const;
    double segmentLengthAlongNodes(const std::vector<Node>& nodes, const Segment& seg) const;
    double distance2D(double x1, double y1, double x2, double y2) const;
};

} // namespace IndustrialNet
