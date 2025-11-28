#include "CanIdFilterLib.h"
#include "qglobal.h"
#include <algorithm>
#include <sstream>
#include <set>
#include <map>
#include <stdexcept>
#include <QDebug>

#ifndef NOMINMAX
#define NOMINMAX
#endif

namespace canopt2 {

namespace {
// 获取最大ID值
uint32_t GetMaxCanId(bool useExtendedId) {
    return useExtendedId ? 0x1FFFFFFF : 0x7FF;
}

// 计算最优掩码
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
    uint32_t varyingBits = 0;

    for (uint32_t id : ids) {
        commonBits &= id;
        varyingBits |= id;
    }

    // 变化位 = 所有出现过的1位 异或 共同位
    varyingBits ^= commonBits;

    // 计算掩码：变化位设为0（不关心），其他位设为1（必须匹配）
    uint32_t mask = ~varyingBits;

    // 确保掩码不超过最大ID值
    mask &= maxIdValue;

    // 计算滤波器ID（基础ID）
    uint32_t filterId = commonBits & mask;

    return std::make_pair(mask, filterId);
}

// 掩码有效性验证
bool IsMaskEffective(const std::vector<uint32_t>& targetIds, uint32_t filterId, uint32_t mask, bool useExtendedId) {
    const uint32_t maxIdValue = GetMaxCanId(useExtendedId);  // 现在使用了这个变量

    // 验证1：所有目标ID都能通过掩码
    for (uint32_t id : targetIds) {
        if ((id & mask) != filterId) {
            return false;  // 有目标ID被过滤掉了
        }
    }

    // 验证2：计算可能误判的ID数量（简化版）
    // 计算掩码中0位的数量（不关心的位数）
    uint32_t careBits = mask;
    int dontCareBitCount = 0;
    for (int i = 0; i < (useExtendedId ? 29 : 11); i++) {
        if (!(careBits & 1)) {
            dontCareBitCount++;
        }
        careBits >>= 1;
    }

    // 计算可能匹配的ID总数
    uint32_t possibleMatches = 1u << dontCareBitCount;

    // 如果可能匹配的ID数量不超过目标ID数量的4倍，认为是有效的
    if (possibleMatches > targetIds.size() * 4) {
        return false;  // 误判率可能太高
    }

    return true;
}
} // namespace
std::vector<IdAllocationResult> AllocateCanIds(
    const std::vector<CanNodeInfo>& nodes,
    const std::vector<CanSignalInfo>& canSignals,  // 修改参数名，避免与Qt宏冲突
    bool useExtendedId,
    uint32_t startId)
{
    std::vector<IdAllocationResult> results;

    try {
        // 收集所有信号类型
        std::set<std::string> signalTypes;
        for (const auto& node : nodes) {
            for (const auto& msgName : node.messageNames) {
                signalTypes.insert(msgName);
            }
        }

        // 为每种信号类型分配基础ID范围
        std::map<std::string, uint32_t> signalBaseIds;
        uint32_t baseId = startId;
        const uint32_t ID_RANGE_SIZE = useExtendedId ? 0x1000 : 0x100; // 扩展ID范围更大

        for (const auto& signalType : signalTypes) {
            signalBaseIds[signalType] = baseId;
            baseId += ID_RANGE_SIZE;
        }

        // 为每个节点的每个发送信号分配具体ID
        std::map<std::string, uint32_t> nodeCounter;
        std::set<uint32_t> usedIds;
        const uint32_t maxIdValue = GetMaxCanId(useExtendedId);  // 添加这行

        for (const auto& node : nodes) {
            for (const auto& signalType : node.messageNames) {
                // 初始化计数器
                if (nodeCounter.find(signalType) == nodeCounter.end()) {
                    nodeCounter[signalType] = 0;
                }

                // 分配ID：基础ID + 节点序号
                uint32_t signalId = signalBaseIds[signalType] + nodeCounter[signalType];

                // 确保ID唯一且不超出范围
                while (usedIds.count(signalId) > 0 || signalId > maxIdValue) {  // 使用maxIdValue
                    signalId++;
                    if (signalId > maxIdValue) {
                        // 如果超出范围，回退到简单递增分配
                        signalId = startId;
                        while (usedIds.count(signalId) > 0 && signalId <= maxIdValue) {
                            signalId++;
                        }
                        if (signalId > maxIdValue) {
                            throw std::runtime_error("CAN ID空间不足");
                        }
                    }
                }

                usedIds.insert(signalId);

                // 创建分配结果
                IdAllocationResult result;
                result.nodeName = node.nodeName;
                result.messageName = signalType;
                result.allocatedId = signalId;

                results.push_back(result);
                nodeCounter[signalType]++;
            }
        }
    }
    catch (const std::exception& e) {
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

        const uint32_t maxIdValue = GetMaxCanId(useExtendedId);

        // 验证所有ID都在有效范围内
        for (uint32_t id : ids) {
            if (id > maxIdValue) {
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
            result.maskOrMaxId = maxIdValue;
            result.note = "单个ID掩码模式（精确匹配）";
            return result;
        }

        // 情况2：尝试掩码模式 - 优先使用掩码模式
        bool maskFound = false;
        uint32_t bestMask = 0;
        uint32_t bestFilterId = 0;
        // size_t bestCoverage = 0;  // 注释掉未使用的变量

        const int totalBits = useExtendedId ? 29 : 11;

        // 尝试不同的掩码位模式
        for (int maskBits = totalBits - 1; maskBits >= 4; maskBits--) {
            uint32_t mask = (0xFFFFFFFFu >> (32 - maskBits)) << (totalBits - maskBits);
            mask &= maxIdValue;

            // 对每个可能的滤波器ID检查覆盖情况
            for (uint32_t testFilterId = 0; testFilterId <= maxIdValue; testFilterId += (1 << (totalBits - maskBits))) {
                std::vector<uint32_t> coveredIds;

                for (uint32_t id : uniqueIds) {
                    if ((id & mask) == (testFilterId & mask)) {
                        coveredIds.push_back(id);
                    }
                }

                // 检查是否覆盖所有目标ID且没有过多误判
                if (coveredIds.size() == uniqueIds.size()) {
                    // 计算可能的误判数量
                    int dontCareBits = totalBits - maskBits;
                    uint32_t possibleMatches = 1u << dontCareBits;

                    // 如果误判率可接受，使用这个掩码
                    if (possibleMatches <= uniqueIds.size() * 8) { // 可调整的阈值
                        maskFound = true;
                        bestMask = mask;
                        bestFilterId = testFilterId & mask;
                        break;
                    }
                }
            }

            if (maskFound) break;
        }

        // 如果找到有效的掩码配置
        if (maskFound) {
            result.mode = "mask";
            result.filterCount = 1;
            result.filterId = bestFilterId;
            result.maskOrMaxId = bestMask;

            std::stringstream note;
            note << "掩码模式，覆盖" << uniqueIds.size() << "个ID";
            note << " (掩码:0x" << std::hex << bestMask << ", ID:0x" << bestFilterId << ")";
            result.note = note.str();
            return result;
        }

        // 情况3：回退到列表模式
        result.mode = "list";
        result.filterCount = static_cast<uint32_t>(uniqueIds.size());
        result.filterId = uniqueIds[0];
        result.maskOrMaxId = uniqueIds.back();

        std::stringstream note;
        note << "列表模式，使用" << uniqueIds.size() << "个滤波器";
        note << " (ID:0x" << std::hex << uniqueIds[0] << "~0x" << uniqueIds.back() << ")";
        result.note = note.str();

    }
    catch (const std::exception& e) {
        result.mode = "error";
        result.note = "设计失败: " + std::string(e.what());
    }

    return result;
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

