/**
 * @file OptionalCanFeatures.hpp
 * @brief CAN网络可选功能模块 - 统一接口版（命名空间 canopt1）
 */

#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include <iostream>

namespace canopt1 {

    // ============================================================================
    // 功能1：收发器斜率控制电阻选择
    // ============================================================================

    struct SlopeControlInput {
        uint32_t baudrate{ 250000 };
        uint32_t timeQuantaPerBit = 10; // 默认10个时间份额
        double targetRiseTimeNs{ 150.0 };
        double cableLengthMeters{ 50.0 };
        double maxRiseTimeRatio{ 0.1 };
        double loadCapacitancePf{ 100.0 };
    };

    struct SlopeControlOutput {
        double recommendedResistorOhm{ 0.0 };
        double actualRiseTimeNs{ 0.0 };
        double targetRiseTimeNs{ 0.0 };
        double bitTimeNs{ 0.0 };
        double riseTimeRatioPct{ 0.0 };

        std::string recommendedMode;
        double modeResistorOhm{ 0.0 };
        double modeRiseTimeNs{ 0.0 };
        double modeFallTimeNs{ 0.0 };
        std::string modeReasoning;

        bool calculationSuccess{ false };
        bool isSuitable{ false };
        std::string statusMessage;
        std::vector<std::string> warnings;
    };

    SlopeControlOutput CalculateSlopeControl(const SlopeControlInput& input);

    // ============================================================================
    // 功能2：报文ID分配
    // ============================================================================

    struct MessageIdAllocationInput {
        bool useExtendedId{ false };
        std::vector<std::pair<std::string, uint32_t>> messages;
    };

    struct MessageIdAllocationOutput {
        std::map<std::string, uint32_t> allocatedIds;
        std::map<std::string, std::string> idBinary;

        uint32_t totalMessages{ 0 };
        uint32_t minId{ 0 };
        uint32_t maxId{ 0 };
        bool useExtendedId{ false };

        bool allocationSuccess{ false };
        std::string statusMessage;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    MessageIdAllocationOutput AllocateMessageIds(const MessageIdAllocationInput& input);

    // ============================================================================
    // 功能3：报文滤波器设计
    // ============================================================================

    enum class FilterMode { kList, kRange, kMask };

    struct FilterDesignInput {
        bool useExtendedId{ false };
        std::vector<uint32_t> acceptedIds;
        FilterMode mode{ FilterMode::kList };
    };

    struct FilterEntry {
        uint32_t filterId{ 0 };
        uint32_t mask{ 0 };
        std::string entryMode;
        std::optional<uint32_t> minId;
        std::optional<uint32_t> maxId;
        std::vector<uint32_t> acceptedIds;
    };

    struct FilterDesignOutput {
        FilterMode mode{ FilterMode::kList };
        std::vector<FilterEntry> entries;
        std::string note;

        uint32_t filterCount{ 0 };
        uint32_t totalAcceptedIds{ 0 };
        bool useExtendedId{ false };

        bool designSuccess{ false };
        std::string statusMessage;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    FilterDesignOutput DesignMessageFilter(const FilterDesignInput& input);
    static uint32_t GetTimeQuantaPerBit(const SlopeControlInput& input);

    // ============================================================================
    // 统一封装结构体（方案A）
    // ============================================================================

    struct Canopt1Input {
        bool enableSlope{ true };
        bool enableIdAllocation{ true };
        bool enableFilter{ true };

        SlopeControlInput slope;
        MessageIdAllocationInput idAlloc;
        FilterDesignInput filter;
    };

    struct Canopt1Output {
        bool slopeExecuted{ false };
        bool idAllocationExecuted{ false };
        bool filterExecuted{ false };

        SlopeControlOutput slope;
        MessageIdAllocationOutput idAlloc;
        FilterDesignOutput filter;

        std::string statusMessage;
        std::vector<std::string> warnings;
        std::vector<std::string> errors;

        bool success{ false };
    };

    // ============================================================================
    // 主统一计算函数
    // ============================================================================

    /**
     * @brief 统一执行所有启用的可选功能
     */
    Canopt1Output CalculateAll(const Canopt1Input& input);

    // ============================================================================
    // 报告文本函数（用于GUI显示）
    // ============================================================================
    std::string GenerateSlopeControlReport(const SlopeControlOutput& output);
    std::string GenerateIdAllocationReport(const MessageIdAllocationOutput& output);
    std::string GenerateFilterDesignReport(const FilterDesignOutput& output);

} // namespace canopt1
