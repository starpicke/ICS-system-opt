/**
 * @file ConfigManager.cpp
 * @brief 配置管理器实现
 */

#include "ConfigManager.h"
#include <QDebug>

namespace config {

ConfigManager::ConfigManager() : lastError("") {
}

bool ConfigManager::ExportConfig(const SystemConfig& config, const QString& filename) {
    QJsonObject root = SerializeConfig(config);

    QJsonDocument doc(root);
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly)) {
        lastError = "无法打开文件进行写入: " + filename;
        qDebug() << lastError;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "配置导出成功:" << filename;
    lastError = "";
    return true;
}

bool ConfigManager::ImportConfig(const QString& filename, SystemConfig& config) {
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly)) {
        lastError = "无法打开文件进行读取: " + filename;
        qDebug() << lastError;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        lastError = "JSON解析失败: 文件格式错误";
        qDebug() << lastError;
        return false;
    }

    if (!doc.isObject()) {
        lastError = "JSON格式错误: 根元素不是对象";
        qDebug() << lastError;
        return false;
    }

    config = DeserializeConfig(doc.object());

    // 验证配置
    auto [valid, error] = ValidateConfig(config);
    if (!valid) {
        lastError = "配置验证失败: " + error;
        qDebug() << lastError;
        return false;
    }

    qDebug() << "配置导入成功:" << filename;
    lastError = "";
    return true;
}

std::pair<bool, QString> ConfigManager::ValidateConfig(const SystemConfig& config) {
    // 验证基本配置
    if (config.projectName.empty()) {
        return { false, "项目名称不能为空" };
    }

    // 验证波特率配置
    if (config.baudRate.desiredLoadPercent <= 0 || config.baudRate.desiredLoadPercent > 100) {
        return { false, "期望负载率必须在1-100%之间" };
    }

    // 验证网络配置
    if (config.network.maxSegmentLength <= 0) {
        return { false, "最大网段长度必须大于0" };
    }

    if (config.network.maxNodesPerSegment <= 0) {
        return { false, "每段最大节点数必须大于0" };
    }

    // 验证位时序配置
    if (config.bitTiming.systemClock == 0) {
        return { false, "系统时钟不能为0" };
    }

    if (config.bitTiming.targetBaudRate == 0) {
        return { false, "目标波特率不能为0" };
    }

    return { true, "" };
}

SystemConfig ConfigManager::CreateDefaultConfig() {
    SystemConfig config;

    config.timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString();

    // 添加一些默认节点数据
    config.baudRate.nodeDataList.push_back(NodeDataConfig(1, 0, 8, 10.0f, 1, "默认消息1"));
    config.baudRate.nodeDataList.push_back(NodeDataConfig(2, 0, 8, 20.0f, 1, "默认消息2"));

    // 添加默认网络节点
    config.network.nodes.push_back(NetworkNodeConfig(1, 0.0, 0.0, 120.0, "节点1"));
    config.network.nodes.push_back(NetworkNodeConfig(2, 50.0, 0.0, 120.0, "节点2"));

    return config;
}

// JSON序列化实现
QJsonObject ConfigManager::SerializeConfig(const SystemConfig& config) {
    QJsonObject root;

    // 基本信息
    root["version"] = QString::fromStdString(config.version);
    root["timestamp"] = QString::fromStdString(config.timestamp);
    root["projectName"] = QString::fromStdString(config.projectName);
    root["author"] = QString::fromStdString(config.author);
    root["description"] = QString::fromStdString(config.description);

    // 各个模块配置
    root["baudRate"] = SerializeBaudRateConfig(config.baudRate);
    root["network"] = SerializeNetworkConfig(config.network);
    root["bitTiming"] = SerializeBitTimingConfig(config.bitTiming);
    root["slopeControl"] = SerializeSlopeControlConfig(config.slopeControl);

    return root;
}

SystemConfig ConfigManager::DeserializeConfig(const QJsonObject& json) {
    SystemConfig config;

    // 基本信息
    if (json.contains("version")) config.version = json["version"].toString().toStdString();
    if (json.contains("timestamp")) config.timestamp = json["timestamp"].toString().toStdString();
    if (json.contains("projectName")) config.projectName = json["projectName"].toString().toStdString();
    if (json.contains("author")) config.author = json["author"].toString().toStdString();
    if (json.contains("description")) config.description = json["description"].toString().toStdString();

    // 各个模块配置
    if (json.contains("baudRate")) {
        config.baudRate = DeserializeBaudRateConfig(json["baudRate"].toObject());
    }
    if (json.contains("network")) {
        config.network = DeserializeNetworkConfig(json["network"].toObject());
    }
    if (json.contains("bitTiming")) {
        config.bitTiming = DeserializeBitTimingConfig(json["bitTiming"].toObject());
    }
    if (json.contains("slopeControl")) {
        config.slopeControl = DeserializeSlopeControlConfig(json["slopeControl"].toObject());
    }

    return config;
}

// 各个配置的序列化实现
QJsonObject ConfigManager::SerializeBaudRateConfig(const SystemConfig::BaudRateConfig& config) {
    QJsonObject obj;
    obj["desiredLoadPercent"] = config.desiredLoadPercent;
    obj["theoreticalBaudRate"] = config.theoreticalBaudRate;
    obj["recommendedBaudRate"] = config.recommendedBaudRate;
    obj["actualLoad"] = config.actualLoad;

    QJsonArray nodesArray;
    for (const auto& node : config.nodeDataList) {
        nodesArray.append(SerializeNodeConfig(node));
    }
    obj["nodes"] = nodesArray;

    return obj;
}

QJsonObject ConfigManager::SerializeNetworkConfig(const SystemConfig::NetworkConfig& config) {
    QJsonObject obj;
    obj["maxSegmentLength"] = config.maxSegmentLength;
    obj["maxNodesPerSegment"] = config.maxNodesPerSegment;
    obj["requiredMinReceiveVoltage"] = config.requiredMinReceiveVoltage;

    QJsonArray nodesArray;
    for (const auto& node : config.nodes) {
        nodesArray.append(SerializeNetworkNodeConfig(node));
    }
    obj["nodes"] = nodesArray;

    return obj;
}

QJsonObject ConfigManager::SerializeBitTimingConfig(const SystemConfig::BitTimingConfig& config) {
    QJsonObject obj;
    obj["systemClock"] = static_cast<qint64>(config.systemClock);
    obj["targetBaudRate"] = static_cast<qint64>(config.targetBaudRate);
    obj["maxErrorPercent"] = config.maxErrorPercent;
    obj["brp"] = config.brp;
    obj["tseg1"] = config.tseg1;
    obj["tseg2"] = config.tseg2;
    obj["sjw"] = config.sjw;
    obj["btrRegister"] = static_cast<qint64>(config.btrRegister);
    return obj;
}

QJsonObject ConfigManager::SerializeSlopeControlConfig(const SlopeControlConfig& config) {
    QJsonObject obj;
    obj["baudrate"] = static_cast<qint64>(config.baudrate);
    obj["targetRiseTimeNs"] = config.targetRiseTimeNs;
    obj["cableLengthMeters"] = config.cableLengthMeters;
    obj["loadCapacitancePf"] = config.loadCapacitancePf;
    obj["calculatedResistance"] = config.calculatedResistance;
    obj["actualRiseTime"] = config.actualRiseTime;
    obj["statusMessage"] = QString::fromStdString(config.statusMessage);
    return obj;
}

// 各个配置的反序列化实现
SystemConfig::BaudRateConfig ConfigManager::DeserializeBaudRateConfig(const QJsonObject& json) {
    SystemConfig::BaudRateConfig config;

    if (json.contains("desiredLoadPercent")) {
        config.desiredLoadPercent = json["desiredLoadPercent"].toInt();
    }
    if (json.contains("theoreticalBaudRate")) {
        config.theoreticalBaudRate = json["theoreticalBaudRate"].toDouble();
    }
    if (json.contains("recommendedBaudRate")) {
        config.recommendedBaudRate = json["recommendedBaudRate"].toDouble();
    }
    if (json.contains("actualLoad")) {
        config.actualLoad = json["actualLoad"].toDouble();
    }

    if (json.contains("nodes")) {
        QJsonArray nodesArray = json["nodes"].toArray();
        for (const auto& nodeValue : nodesArray) {
            config.nodeDataList.push_back(DeserializeNodeConfig(nodeValue.toObject()));
        }
    }

    return config;
}

SystemConfig::NetworkConfig ConfigManager::DeserializeNetworkConfig(const QJsonObject& json) {
    SystemConfig::NetworkConfig config;

    if (json.contains("maxSegmentLength")) {
        config.maxSegmentLength = json["maxSegmentLength"].toDouble();
    }
    if (json.contains("maxNodesPerSegment")) {
        config.maxNodesPerSegment = json["maxNodesPerSegment"].toInt();
    }
    if (json.contains("requiredMinReceiveVoltage")) {
        config.requiredMinReceiveVoltage = json["requiredMinReceiveVoltage"].toDouble();
    }

    if (json.contains("nodes")) {
        QJsonArray nodesArray = json["nodes"].toArray();
        for (const auto& nodeValue : nodesArray) {
            config.nodes.push_back(DeserializeNetworkNodeConfig(nodeValue.toObject()));
        }
    }

    return config;
}

SystemConfig::BitTimingConfig ConfigManager::DeserializeBitTimingConfig(const QJsonObject& json) {
    SystemConfig::BitTimingConfig config;

    if (json.contains("systemClock")) {
        config.systemClock = json["systemClock"].toVariant().toUInt();
    }
    if (json.contains("targetBaudRate")) {
        config.targetBaudRate = json["targetBaudRate"].toVariant().toUInt();
    }
    if (json.contains("maxErrorPercent")) {
        config.maxErrorPercent = json["maxErrorPercent"].toDouble();
    }
    if (json.contains("brp")) {
        config.brp = json["brp"].toInt();
    }
    if (json.contains("tseg1")) {
        config.tseg1 = json["tseg1"].toInt();
    }
    if (json.contains("tseg2")) {
        config.tseg2 = json["tseg2"].toInt();
    }
    if (json.contains("sjw")) {
        config.sjw = json["sjw"].toInt();
    }
    if (json.contains("btrRegister")) {
        config.btrRegister = json["btrRegister"].toVariant().toUInt();
    }

    return config;
}

SlopeControlConfig ConfigManager::DeserializeSlopeControlConfig(const QJsonObject& json) {
    SlopeControlConfig config;

    if (json.contains("baudrate")) {
        config.baudrate = json["baudrate"].toVariant().toUInt();
    }
    if (json.contains("targetRiseTimeNs")) {
        config.targetRiseTimeNs = json["targetRiseTimeNs"].toDouble();
    }
    if (json.contains("cableLengthMeters")) {
        config.cableLengthMeters = json["cableLengthMeters"].toDouble();
    }
    if (json.contains("loadCapacitancePf")) {
        config.loadCapacitancePf = json["loadCapacitancePf"].toDouble();
    }
    if (json.contains("calculatedResistance")) {
        config.calculatedResistance = json["calculatedResistance"].toDouble();
    }
    if (json.contains("actualRiseTime")) {
        config.actualRiseTime = json["actualRiseTime"].toDouble();
    }
    if (json.contains("statusMessage")) {
        config.statusMessage = json["statusMessage"].toString().toStdString();
    }

    return config;
}

// 节点配置序列化
QJsonObject ConfigManager::SerializeNodeConfig(const NodeDataConfig& node) {
    QJsonObject obj;
    obj["nodeId"] = node.nodeId;
    obj["frameType"] = node.frameType;
    obj["dataBytes"] = node.dataBytes;
    obj["sendTimeMs"] = node.sendTimeMs;
    obj["nodeCount"] = node.nodeCount;
    obj["messageName"] = QString::fromStdString(node.messageName);
    obj["priority"] = node.priority;           // 新增优先级字段
    obj["maxLength"] = node.maxLength;         // 新增最大长度字段
    return obj;
}

QJsonObject ConfigManager::SerializeNetworkNodeConfig(const NetworkNodeConfig& node) {
    QJsonObject obj;
    obj["id"] = node.id;
    obj["x"] = node.x;
    obj["y"] = node.y;
    obj["inputImpedance"] = node.inputImpedance;
    obj["nodeName"] = QString::fromStdString(node.nodeName);

    // 序列化发送消息列表
    QJsonArray sendMessagesArray;
    for (const auto& msg : node.sendMessages) {
        sendMessagesArray.append(QString::fromStdString(msg));
    }
    obj["sendMessages"] = sendMessagesArray;

    // 序列化接收消息列表
    QJsonArray receiveMessagesArray;
    for (const auto& msg : node.receiveMessages) {
        receiveMessagesArray.append(QString::fromStdString(msg));
    }
    obj["receiveMessages"] = receiveMessagesArray;

    return obj;
}

// 节点配置反序列化
NodeDataConfig ConfigManager::DeserializeNodeConfig(const QJsonObject& json) {
    NodeDataConfig node;

    if (json.contains("nodeId")) node.nodeId = json["nodeId"].toInt();
    if (json.contains("frameType")) node.frameType = json["frameType"].toInt();
    if (json.contains("dataBytes")) node.dataBytes = json["dataBytes"].toInt();
    if (json.contains("sendTimeMs")) node.sendTimeMs = json["sendTimeMs"].toDouble();
    if (json.contains("nodeCount")) node.nodeCount = json["nodeCount"].toInt();
    if (json.contains("messageName")) node.messageName = json["messageName"].toString().toStdString();
    if (json.contains("priority")) node.priority = json["priority"].toInt();           // 新增优先级字段
    if (json.contains("maxLength")) node.maxLength = json["maxLength"].toDouble();     // 新增最大长度字段

    return node;
}

NetworkNodeConfig ConfigManager::DeserializeNetworkNodeConfig(const QJsonObject& json) {
    NetworkNodeConfig node;

    if (json.contains("id")) node.id = json["id"].toInt();
    if (json.contains("x")) node.x = json["x"].toDouble();
    if (json.contains("y")) node.y = json["y"].toDouble();
    if (json.contains("inputImpedance")) node.inputImpedance = json["inputImpedance"].toDouble();
    if (json.contains("nodeName")) node.nodeName = json["nodeName"].toString().toStdString();

    // 反序列化发送消息列表
    if (json.contains("sendMessages")) {
        QJsonArray sendArray = json["sendMessages"].toArray();
        for (const auto& msgValue : sendArray) {
            node.sendMessages.push_back(msgValue.toString().toStdString());
        }
    }

    // 反序列化接收消息列表
    if (json.contains("receiveMessages")) {
        QJsonArray receiveArray = json["receiveMessages"].toArray();
        for (const auto& msgValue : receiveArray) {
            node.receiveMessages.push_back(msgValue.toString().toStdString());
        }
    }

    return node;
}


} // namespace config

