#pragma once
/**
 * @file ConfigData.h
 * @brief 配置数据导入导出结构
 */

#include <string>
#include <vector>
#include <map>

namespace config {

// 波特率计算节点配置
struct NodeDataConfig {
    int nodeId = 0;
    int frameType = 0;  // 0=标准帧, 1=扩展帧
    int dataBytes = 8;
    float sendTimeMs = 10.0f;
    int nodeCount = 1;
    std::string messageName = "消息";
    int priority = 0;           // 添加优先级字段
    float maxLength = 0.0f;     // 添加最大长度字段

    NodeDataConfig() = default;
    NodeDataConfig(int id, int type, int bytes, float time, int count,
                   const std::string& name = "消息", int prio = 0, float maxLen = 0.0f)
        : nodeId(id), frameType(type), dataBytes(bytes), sendTimeMs(time),
        nodeCount(count), messageName(name), priority(prio), maxLength(maxLen) {}
};

// 网络设计节点配置
struct NetworkNodeConfig {
    int id = 0;
    double x = 0.0;
    double y = 0.0;
    double inputImpedance = 120.0;
    std::string nodeName = "节点";
    std::vector<std::string> sendMessages;     // 添加发送消息列表
    std::vector<std::string> receiveMessages;  // 添加接收消息列表

    NetworkNodeConfig() = default;
    NetworkNodeConfig(int nodeId, double posX, double posY, double impedance = 120.0,
                      const std::string& name = "节点",
                      const std::vector<std::string>& sendMsgs = {},
                      const std::vector<std::string>& recvMsgs = {})
        : id(nodeId), x(posX), y(posY), inputImpedance(impedance),
        nodeName(name), sendMessages(sendMsgs), receiveMessages(recvMsgs) {}
};

// 斜率控制配置
struct SlopeControlConfig {
    uint32_t baudrate = 500000;
    double targetRiseTimeNs = 200.0;
    double cableLengthMeters = 50.0;
    double loadCapacitancePf = 100.0;
    double calculatedResistance = 0.0;
    double actualRiseTime = 0.0;
    std::string statusMessage = "就绪";
};

// 完整配置结构
struct SystemConfig {
    // 基本信息
    std::string version = "2.0";
    std::string timestamp;
    std::string projectName = "CAN总线网络设计";
    std::string author = "用户";
    std::string description = "CAN总线网络参数优化系统配置";

    // 波特率计算配置
    struct BaudRateConfig {
        std::vector<NodeDataConfig> nodeDataList;
        int desiredLoadPercent = 50;
        double theoreticalBaudRate = 0.0;
        double recommendedBaudRate = 0.0;
        double actualLoad = 0.0;
    } baudRate;

    // 网络设计配置
    struct NetworkConfig {
        std::vector<NetworkNodeConfig> nodes;
        double maxSegmentLength = 100.0;
        int maxNodesPerSegment = 32;
        double requiredMinReceiveVoltage = 0.9;
    } network;

    // 位时序配置
    struct BitTimingConfig {
        uint32_t systemClock = 48000000;
        uint32_t targetBaudRate = 500000;
        double maxErrorPercent = 1.0;
        int brp = 0;
        int tseg1 = 0;
        int tseg2 = 0;
        int sjw = 0;
        uint32_t btrRegister = 0;
    } bitTiming;

    // 斜率控制配置
    SlopeControlConfig slopeControl;
};

} // namespace config
