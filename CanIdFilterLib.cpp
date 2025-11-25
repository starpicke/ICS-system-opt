/**
 * @file CanIdFilterLib.cpp
 * @brief CAN ID分配和滤波器设计模块 - 实现文件
 */

#include "CanIdFilterLib.h"
#include <algorithm>
#include <sstream>
#include <set>
#include <map>
#include <stdexcept>

 // 防止windows.h中的max/min宏定义冲突
#ifndef NOMINMAX
#define NOMINMAX
#endif

namespace canopt2 {

    namespace {
        // 获取最大ID值
        uint32_t GetMaxCanId(bool useExtendedId) {
            return useExtendedId ? 0x1FFFFFFF : 0x7FF;
        }

        // 判断ID列表是否连续 - 使用结构体返回值
        struct RangeCheckResult {
            bool isContinuous;
            uint32_t minIdValue;
            uint32_t maxIdValue;
        };

        RangeCheckResult CheckContinuousRange(const std::vector<uint32_t>& ids) {
            RangeCheckResult result;
            result.isContinuous = false;
            result.minIdValue = 0;
            result.maxIdValue = 0;

            if (ids.empty()) return result;

            std::vector<uint32_t> sortedIds = ids;
            std::sort(sortedIds.begin(), sortedIds.end());

            result.minIdValue = sortedIds.front();
            result.maxIdValue = sortedIds.back();

            // 检查是否连续
            for (size_t i = 1; i < sortedIds.size(); ++i) {
                if (sortedIds[i] != sortedIds[i - 1] + 1) {
                    return result;
                }
            }

            result.isContinuous = true;
            return result;
        }

        // 计算掩码模式
        std::pair<uint32_t, uint32_t> CalculateMaskPattern(const std::vector<uint32_t>& ids, bool useExtendedId) {
            if (ids.empty()) {
                return std::make_pair(0u, 0u);
            }

            uint32_t commonBits = ~0u;
            uint32_t varyingBits = 0;

            for (size_t i = 0; i < ids.size(); ++i) {
                uint32_t id = ids[i];
                commonBits &= id;
                varyingBits |= (ids[0] ^ id);
            }

            // 计算掩码：变化位为0，共同位为1
            uint32_t mask = 0;
            uint32_t temp = varyingBits;
            while (temp) {
                mask = (mask << 1) | 1;
                temp >>= 1;
            }
            mask = ~mask;

            // 应用ID范围限制
            const uint32_t maxMaskValue = GetMaxCanId(useExtendedId);
            mask &= maxMaskValue;

            return std::make_pair(mask, commonBits & mask);
        }
    }

    std::vector<IdAllocationResult> AllocateCanIds(
        const std::vector<CanNodeInfo>& nodes,
        const std::vector<CanSignalInfo>& signals,
        bool useExtendedId,
        uint32_t startId)
    {
        std::vector<IdAllocationResult> results;

        // 参数验证
        if (nodes.empty() || signals.empty()) {
            return results;
        }

        try {
            // 按优先级排序信号（数值越小优先级越高）
            std::vector<CanSignalInfo> sortedSignals = signals;
            std::sort(sortedSignals.begin(), sortedSignals.end(),
                [](const CanSignalInfo& a, const CanSignalInfo& b) {
                    return a.priority < b.priority;
                });

            uint32_t currentId = startId;
            uint32_t maximumId = GetMaxCanId(useExtendedId);
            std::set<uint32_t> usedIds;

            // 为每个信号分配ID
            for (size_t i = 0; i < sortedSignals.size(); ++i) {
                const CanSignalInfo& signal = sortedSignals[i];

                // 查找包含该信号的节点
                for (size_t j = 0; j < nodes.size(); ++j) {
                    const CanNodeInfo& node = nodes[j];

                    if (std::find(node.messageNames.begin(), node.messageNames.end(),
                        signal.messageName) != node.messageNames.end()) {

                        // 检查ID是否超出范围
                        if (currentId > maximumId) {
                            return results; // ID空间不足
                        }

                        // 检查ID是否已被使用
                        while (usedIds.count(currentId) > 0) {
                            currentId++;
                            if (currentId > maximumId) {
                                return results; // ID空间不足
                            }
                        }

                        // 分配ID
                        IdAllocationResult result;
                        result.nodeName = node.nodeName;
                        result.messageName = signal.messageName;
                        result.allocatedId = currentId;

                        results.push_back(result);
                        usedIds.insert(currentId);
                        currentId++;

                        break; // 一个信号只需要分配一次ID
                    }
                }
            }
        }
        catch (const std::exception& e) {
            // 异常处理
            results.clear();
        }

        return results;
    }

    FilterDesignResult DesignCanFilter(
        const std::vector<uint32_t>& ids,
        bool useExtendedId)
    {
        FilterDesignResult result;

        try {
            if (ids.empty()) {
                result.note = "ID列表为空";
                return result;
            }

            const uint32_t maximumIdValue = GetMaxCanId(useExtendedId);

            // 验证所有ID都在有效范围内
            for (size_t i = 0; i < ids.size(); ++i) {
                uint32_t id = ids[i];
                if (id > maximumIdValue) {
                    result.note = "ID超出有效范围";
                    return result;
                }
            }

            // 去重并排序
            std::vector<uint32_t> uniqueIds = ids;
            std::sort(uniqueIds.begin(), uniqueIds.end());
            std::vector<uint32_t>::iterator last = std::unique(uniqueIds.begin(), uniqueIds.end());
            uniqueIds.erase(last, uniqueIds.end());

            // 尝试范围模式 - 使用结构体返回值
            RangeCheckResult rangeResult = CheckContinuousRange(uniqueIds);
            if (rangeResult.isContinuous) {
                result.mode = "range";
                result.filterCount = 1;
                result.filterId = rangeResult.minIdValue;
                result.maskOrMaxId = rangeResult.maxIdValue;
                result.note = "推荐范围滤波";
                return result;
            }

            // 尝试掩码模式
            std::pair<uint32_t, uint32_t> maskResult = CalculateMaskPattern(uniqueIds, useExtendedId);
            uint32_t mask = maskResult.first;
            uint32_t filterId = maskResult.second;

            if (mask != 0 && mask != GetMaxCanId(useExtendedId)) {
                result.mode = "mask";
                result.filterCount = 1;
                result.filterId = filterId;
                result.maskOrMaxId = mask;
                result.note = "推荐掩码滤波";
                return result;
            }

            // 回退到列表模式
            result.mode = "list";
            result.filterCount = static_cast<uint32_t>(uniqueIds.size());
            result.filterId = uniqueIds.front();
            result.maskOrMaxId = uniqueIds.back();
            result.note = "使用列表模式";

        }
        catch (const std::exception& e) {
            result.note = "设计失败: " + std::string(e.what());
        }

        return result;
    }

    std::string GenerateIdAllocationReport(const std::vector<IdAllocationResult>& results) {
        std::ostringstream oss;

        for (size_t i = 0; i < results.size(); ++i) {
            const IdAllocationResult& result = results[i];
            oss << "节点: " << result.nodeName
                << ", 信号: " << result.messageName
                << ", ID: 0x" << std::hex << result.allocatedId << std::dec;

            if (i < results.size() - 1) {
                oss << "\n";
            }
        }

        return oss.str();
    }

    std::string GenerateFilterDesignReport(const FilterDesignResult& result) {
        std::ostringstream oss;
        oss << "=== 滤波器设计 ===" << "\n";
        oss << "模式: " << result.mode << "\n";
        oss << "数量: " << result.filterCount << "\n";

        if (result.mode == "range") {
            oss << "MinId: 0x" << std::hex << result.filterId << std::dec << "\n";
            oss << "MaxId: 0x" << std::hex << result.maskOrMaxId << std::dec << "\n";
        }
        else if (result.mode == "mask") {
            oss << "FilterId: 0x" << std::hex << result.filterId << std::dec << "\n";
            oss << "Mask/Max: 0x" << std::hex << result.maskOrMaxId << std::dec << "\n";
        }
        else if (result.mode == "list") {
            oss << "FirstId: 0x" << std::hex << result.filterId << std::dec << "\n";
            oss << "LastId: 0x" << std::hex << result.maskOrMaxId << std::dec << "\n";
        }

        oss << "说明: " << result.note;

        return oss.str();
    }

} // namespace canopt2