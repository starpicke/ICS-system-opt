/**
 * @file ComprehensiveReport.h
 * @brief 综合报告数据结构 - 整合所有模块
 */

#pragma once
#include <string>
#include <vector>
#include <map>
#include <ctime>

namespace canproject {

// 先定义所有需要用到的结构体
struct NodeData {
    int nodeId;
    int frameType;  // 0=标准帧, 1=扩展帧
    int dataBytes;
    float sendTimeMs;
    int nodeCount;
};

// 网络设计相关的结构体定义
struct Node {
    int id;
    double x, y;
    double inputImpedance;
};

struct Segment {
    int id;
    double startX, startY;
    double endX, endY;
    std::vector<int> nodeIds;
    double length;
    double estimatedVoltage;
};

// 第一问：波特率计算模块
struct BaudRateCalculation {
    struct Input {
        std::vector<NodeData> nodeDataList;
        int desiredLoadPercent;
    } input;

    struct Output {
        float totalBitRate;
        float requiredBaudRate;
        float recommendedBaudRate;
        float actualLoadPercent;
        std::string warningMessage;
        bool calculationSuccess;
    } output;
};

// 第二问：网络设计模块
struct NetworkDesign {
    struct Input {
        std::vector<Node> nodes;
        double maxSegmentLength;
        int maxNodesPerSegment;
        double requiredMinReceiveVoltage;
    } input;

    struct Output {
        std::vector<Segment> segments;
        struct DevicePlacement {
            std::vector<std::pair<double, double>> terminators;
            std::vector<std::pair<double, double>> repeaters;
            std::vector<std::pair<double, double>> bridges;
        } devices;
        std::vector<std::pair<int, bool>> nodeReceiveStatus;
        bool overallSuccess;
        std::vector<std::string> logs;
    } output;
};

// 第三问：位时序计算模块
struct BitTimingCalculation {
    struct Input {
        uint32_t systemClock;
        uint32_t targetBaudRate;
        double maxErrorPercent;
    } input;

    struct Output {
        uint32_t BRP;
        uint32_t SJW;
        uint32_t TSEG1;
        uint32_t TSEG2;
        uint32_t btrRegister;
        uint32_t actualBaudRate;
        double errorPercent;
        bool calculationSuccess;
        std::string statusMessage;
    } output;
};

// 第四问：斜率控制模块
struct SlopeControl {
    struct Input {
        uint32_t baudrate;
        double targetRiseTimeNs;
        double cableLengthMeters;
        double loadCapacitancePf;
    } input;

    struct Output {
        double recommendedResistor;
        double actualRiseTimeNs;
        std::string recommendedMode;
        std::string modeReasoning;
        bool calculationSuccess;
        std::string statusMessage;
    } output;
};

// 综合报告结构
struct ComprehensiveReport {
    // 项目信息
    std::string projectName;
    std::string author;
    std::string timestamp;
    std::string description;

    // 各模块结果
    BaudRateCalculation baudRate;
    NetworkDesign network;
    BitTimingCalculation bitTiming;
    SlopeControl slopeControl;

    // 总体状态
    bool allCalculationsSuccessful;
    std::vector<std::string> overallWarnings;
    std::vector<std::string> overallErrors;

    // 数据收集方法
    void CollectAllData();
    void UpdateTimestamp();
    void ValidateOverallStatus();
};

} // namespace canproject
