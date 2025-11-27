/**
 * @file CanIdFilterLib.h
 * @brief CAN ID分配和滤波器设计模块 - 头文件
 *
 * 本模块实现两个核心功能：
 * 1. CAN ID分配 - 根据节点和信号信息自动分配CAN ID
 * 2. 滤波器设计 - 设计CAN接收滤波器配置（列表/范围/掩码模式）
 *
 * 设计特点：
 * - 避免与Qt项目中已有的NodeInfo和BaudRate结构体冲突
 * - 使用独立的结构体命名，便于Qt整合
 * - 支持标准ID（11位）和扩展ID（29位）两种模式
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace canopt2 {

    // ============================================================================
    // 核心数据结构（避免与Qt项目中的结构体冲突）
    // ============================================================================

    /**
     * @struct CanNodeInfo
     * @brief CAN节点信息（避免与Qt中的NodeInfo冲突）
     */
    struct CanNodeInfo {
        std::string nodeName;                      ///< 节点名称
        std::vector<std::string> messageNames;     ///< 节点包含的消息名称列表
    };

    /**
     * @struct CanSignalInfo
     * @brief CAN信号信息（避免与Qt中的BaudRate冲突）
     */
    struct CanSignalInfo {
        std::string messageName;                   ///< 消息名称
        int priority;                              ///< 优先级（数值越小优先级越高）
        bool useExtendedId;                        ///< 是否使用扩展ID
    };

    /**
     * @struct IdAllocationResult
     * @brief ID分配结果
     */
    struct IdAllocationResult {
        std::string nodeName;                      ///< 节点名称
        std::string messageName;                   ///< 消息名称
        uint32_t allocatedId;                      ///< 分配的ID
    };

    /**
     * @struct FilterDesignResult
     * @brief 滤波器设计结果
     */
    struct FilterDesignResult {
        std::string mode;                          ///< 滤波模式："list"/"range"/"mask"
        uint32_t filterCount;                      ///< 滤波器数量
        uint32_t filterId;                         ///< 滤波器ID（列表/范围/掩码模式）
        uint32_t maskOrMaxId;                      ///< 掩码值或最大ID（范围模式）
        std::string note;                          ///< 配置说明
    };

    // ============================================================================
    // 函数接口
    // ============================================================================

    /**
     * @brief 分配CAN ID
     * @param nodes 节点信息列表
     * @param m_signals 信号信息列表
     * @param useExtendedId 是否使用扩展ID
     * @param startId 起始ID
     * @return ID分配结果列表
     */
    std::vector<IdAllocationResult> AllocateCanIds(
        const std::vector<CanNodeInfo>& nodes,
        const std::vector<CanSignalInfo>& m_signals,
        bool useExtendedId,
        uint32_t startId = 0
    );

    /**
     * @brief 设计CAN接收滤波器
     * @param ids 要接受的ID列表
     * @param useExtendedId 是否使用扩展ID
     * @return 滤波器设计结果
     */
    FilterDesignResult DesignCanFilter(
        const std::vector<uint32_t>& ids,
        bool useExtendedId
    );

    /**
     * @brief 生成ID分配报告
     * @param results ID分配结果
     * @return 格式化的报告字符串
     */
    std::string GenerateIdAllocationReport(const std::vector<IdAllocationResult>& results);

    /**
     * @brief 生成滤波器设计报告
     * @param result 滤波器设计结果
     * @return 格式化的报告字符串
     */
    std::string GenerateFilterDesignReport(const FilterDesignResult& result);

} // namespace canopt2
