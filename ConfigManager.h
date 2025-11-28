#pragma once
/**
 * @file ConfigManager.h
 * @brief 配置管理器 - 负责导入导出所有输入配置
 */

#pragma once
#include "ConfigData.h"
#include <QString>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>

namespace config {

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager() = default;

    /**
         * @brief 导出配置到JSON文件
         * @param config 配置数据
         * @param filename 输出文件名
         * @return 导出是否成功
         */
    bool ExportConfig(const SystemConfig& config, const QString& filename);

    /**
         * @brief 从JSON文件导入配置
         * @param filename 输入文件名
         * @param config 输出的配置数据
         * @return 导入是否成功
         */
    bool ImportConfig(const QString& filename, SystemConfig& config);

    /**
         * @brief 验证配置数据的完整性
         * @param config 配置数据
         * @return 验证结果和错误信息
         */
    std::pair<bool, QString> ValidateConfig(const SystemConfig& config);

    /**
         * @brief 创建默认配置
         * @return 默认配置数据
         */
    SystemConfig CreateDefaultConfig();

    /**
         * @brief 获取最后错误信息
         * @return 错误描述
         */
    QString GetLastError() const { return lastError; }

private:
    // JSON序列化方法
    QJsonObject SerializeConfig(const SystemConfig& config);
    SystemConfig DeserializeConfig(const QJsonObject& json);

    // 各个配置的序列化
    QJsonObject SerializeBaudRateConfig(const SystemConfig::BaudRateConfig& config);
    QJsonObject SerializeNetworkConfig(const SystemConfig::NetworkConfig& config);
    QJsonObject SerializeBitTimingConfig(const SystemConfig::BitTimingConfig& config);
    QJsonObject SerializeSlopeControlConfig(const SystemConfig::SlopeControlConfig& config);

    // 各个配置的反序列化
    SystemConfig::BaudRateConfig DeserializeBaudRateConfig(const QJsonObject& json);
    SystemConfig::NetworkConfig DeserializeNetworkConfig(const QJsonObject& json);
    SystemConfig::BitTimingConfig DeserializeBitTimingConfig(const QJsonObject& json);
    SystemConfig::SlopeControlConfig DeserializeSlopeControlConfig(const QJsonObject& json);

    // 节点配置序列化
    QJsonObject SerializeNodeConfig(const NodeDataConfig& node);
    QJsonObject SerializeNetworkNodeConfig(const NetworkNodeConfig& node);

    // 节点配置反序列化
    NodeDataConfig DeserializeNodeConfig(const QJsonObject& json);
    NetworkNodeConfig DeserializeNetworkNodeConfig(const QJsonObject& json);

    QString lastError;
};

} // namespace config

