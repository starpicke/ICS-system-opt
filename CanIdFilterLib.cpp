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


// 新增辅助函数：计算最优掩码
std::pair<uint32_t, uint32_t> CalculateOptimalMask(const std::vector<uint32_t>& ids, bool useExtendedId) {
    if (ids.empty()) {
        return std::make_pair(0u, 0u);
    }

    const uint32_t maxIdValue = GetMaxCanId(useExtendedId);

    // 如果只有一个ID，返回全掩码
    if (ids.size() == 1) {
        return std::make_pair(maxIdValue, ids[0]);
    }

    // 找出所有ID的共同位
    uint32_t commonBits = ~0u;
    for (uint32_t id : ids) {
        commonBits &= id;
    }

    // 找出变化位
    uint32_t varyingBits = 0;
    for (uint32_t id : ids) {
        varyingBits |= (id ^ commonBits);
    }

    // 计算掩码：变化位设为0（不关心），其他位设为1（必须匹配）
    uint32_t mask = ~varyingBits;

    // 确保掩码不超过最大ID值
    mask &= maxIdValue;

    // 计算滤波器ID（基础ID）
    uint32_t filterId = commonBits & mask;

    return std::make_pair(mask, filterId);
}

// 新增辅助函数：验证掩码有效性
bool IsMaskEffective(const std::vector<uint32_t>& targetIds, uint32_t filterId, uint32_t mask) {
    // 验证1：所有目标ID都能通过掩码
    for (uint32_t id : targetIds) {
        if ((id & mask) != filterId) {
            return false;  // 有目标ID被过滤掉了
        }
    }

    // 验证2：计算误判率（被错误接收的ID数量）
    // 简单验证：如果掩码太宽（比如少于4位被屏蔽），可能误判率太高
    uint32_t maskedBits = ~mask;
    int maskedBitCount = 0;
    while (maskedBits) {
        maskedBitCount += (maskedBits & 1);
        maskedBits >>= 1;
    }

    // 如果掩码屏蔽的位数太少（小于4位），可能误判率太高，不推荐使用
    if (maskedBitCount < 4) {
        return false;
    }

    return true;
}}
std::vector<IdAllocationResult> AllocateCanIds(
    const std::vector<CanNodeInfo>& nodes,
    const std::vector<CanSignalInfo>& signals,  // 修正1：参数名改为signals
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
        std::set<std::string> allocatedSignals;  // 新增：记录已分配的信号名称

        // 为每个信号分配ID
        for (size_t i = 0; i < sortedSignals.size(); ++i) {
            const CanSignalInfo& signal = sortedSignals[i];  // 修正2：变量名改为signal

            // 修正3：检查信号是否已经分配过
           // if (allocatedSignals.find(signal.messageName) != allocatedSignals.end()) {
           //     continue; // 如果已经分配过，跳过这个信号
           // }

            bool foundNode = false;

            // 查找包含该信号的节点
            for (size_t j = 0; j < nodes.size(); ++j) {
                const CanNodeInfo& node = nodes[j];

                if (std::find(node.messageNames.begin(), node.messageNames.end(),
                              signal.messageName) != node.messageNames.end()) {

                    foundNode = true;

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
                    allocatedSignals.insert(signal.messageName);  // 记录已分配的信号
                    currentId++;

                    //break; // 一个信号只需要分配一次ID
                }
            }

            // 修正4：如果信号没有被任何节点包含，也分配一个ID（可选）
            if (!foundNode) {
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

                // 分配ID（没有对应节点）
                IdAllocationResult result;
                result.nodeName = "未指定";
                result.messageName = signal.messageName;
                result.allocatedId = currentId;

                results.push_back(result);
                usedIds.insert(currentId);
                allocatedSignals.insert(signal.messageName);
                currentId++;
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
            result.mode = "none";
            result.filterCount = 0;
            result.filterId = 0;
            result.maskOrMaxId = 0;
            result.note = "没有需要接收的ID";
            return result;
        }

        const uint32_t maximumIdValue = GetMaxCanId(useExtendedId);

        // 验证所有ID都在有效范围内
        for (uint32_t id : ids) {
            if (id > maximumIdValue) {
                result.mode = "error";
                result.note = "ID超出有效范围: 0x" + std::to_string(id);
                return result;
            }
        }

        // 去重并排序
        std::vector<uint32_t> uniqueIds = ids;
        std::sort(uniqueIds.begin(), uniqueIds.end());
        auto last = std::unique(uniqueIds.begin(), uniqueIds.end());
        uniqueIds.erase(last, uniqueIds.end());

        // 情况1：单个ID - 使用掩码模式（全掩码精确匹配）
        if (uniqueIds.size() == 1) {
            result.mode = "mask";
            result.filterCount = 1;
            result.filterId = uniqueIds[0];
            result.maskOrMaxId = maximumIdValue;  // 全掩码，精确匹配
            result.note = "单个ID掩码模式（精确匹配）";
            return result;
        }

        // 情况2：尝试掩码模式（优先）
        std::pair<uint32_t, uint32_t> maskResult = CalculateOptimalMask(uniqueIds, useExtendedId);
        uint32_t mask = maskResult.first;
        uint32_t filterId = maskResult.second;

        if (mask != 0 && mask != maximumIdValue) {
            // 验证掩码模式是否有效且高效（能覆盖所有目标ID且误判率低）
            if (IsMaskEffective(uniqueIds, filterId, mask)) {
                result.mode = "mask";
                result.filterCount = 1;
                result.filterId = filterId;
                result.maskOrMaxId = mask;
                result.note = "掩码模式，覆盖" + std::to_string(uniqueIds.size()) + "个ID";
                return result;
            }
        }

        // 情况3：回退到列表模式
        result.mode = "list";
        result.filterCount = static_cast<uint32_t>(uniqueIds.size());
        result.filterId = uniqueIds[0];
        result.maskOrMaxId = uniqueIds.back();
        result.note = "列表模式，使用" + std::to_string(uniqueIds.size()) + "个滤波器";

    }
    catch (const std::exception& e) {
        result.mode = "error";
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
