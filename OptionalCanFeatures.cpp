/**
 * @file OptionalCanFeatures.cpp
 * @brief CAN网络可选功能模块 - 实现文件（命名空间 canopt1）
 *
 * 与 OptionalCanFeatures.hpp 配套，提供：
 *  - 斜率控制电阻选择
 *  - 报文 ID 分配
 *  - 报文滤波器设计
 *  - 一个统一入口 CalculateAll，根据输入 enable 标志执行对应功能
 */

#include "OptionalCanFeatures.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace canopt1 {

    // ----------------------------------------------------------------------------
    // 局部常量与帮助函数（斜率控制）
    // ----------------------------------------------------------------------------
    namespace {
        // 常用阻值列表（E24等常见）——可根据实际库存扩展
        const std::vector<double> kCommonResistors = {
        10, 15, 20, 22, 27, 30, 33, 39, 47, 51, 56, 68, 75, 82, 100,
        120, 150, 200, 220, 270, 330, 390, 470, 510, 560, 680, 750,
        820, 1000, 1200, 1500, 2000
        };

        // 斜率模式配置表
        constexpr struct SlopeProfile {
            const char* mode;
            double resistor;
            double riseTime;
            double fallTime;
            double maxBaudrate;
            double minCableLength;
        } kSlopeProfiles[] = {
            {"slow",   120.0, 300.0, 300.0, 125000.0, 100.0},
            {"normal", 47.0,  150.0, 150.0, 500000.0,  50.0},
            {"fast",   10.0,   50.0,  50.0,1000000.0,   0.0}
        };

        constexpr double kRiseTimeFactor = 2.2; // 10%-90% 上升时间近似系数

        static double CalculateRiseTimeNs(double resistanceOhm, double loadCapacitancePf) {
            const double capacitanceF = loadCapacitancePf * 1e-12;
            // t_r = k * R * C, 转换到 ns 单位
            return kRiseTimeFactor * resistanceOhm * capacitanceF * 1e9;
        }
    } // namespace

    // ----------------------------------------------------------------------------
    // 斜率控制实现
    // ----------------------------------------------------------------------------
    SlopeControlOutput CalculateSlopeControl(const SlopeControlInput& input) {
        SlopeControlOutput output;

        try {
            // 输入验证
            if (input.baudrate == 0) {
                output.statusMessage = "波特率必须大于0";
                return output;
            }
            if (input.targetRiseTimeNs <= 0.0) {
                output.statusMessage = "目标上升时间必须大于0";
                return output;
            }
            if (input.loadCapacitancePf <= 0.0) {
                output.statusMessage = "负载电容必须大于0";
                return output;
            }

            // 位时间（ns）
            const double bitTimeNs = 1e9 / static_cast<double>(input.baudrate);
            output.bitTimeNs = bitTimeNs;

            // 最大允许上升时间 = maxRiseTimeRatio * 位时间
            const double maxRiseTimeNs = bitTimeNs * input.maxRiseTimeRatio;

            // 限制目标上升时间
            const double clampedTarget = std::min(input.targetRiseTimeNs, maxRiseTimeNs);
            output.targetRiseTimeNs = clampedTarget;
            if (input.targetRiseTimeNs > maxRiseTimeNs) {
                std::ostringstream os;
                os << "目标上升时间 " << input.targetRiseTimeNs << " ns 超过允许值，已调整为 "
                    << clampedTarget << " ns";
                output.warnings.push_back(os.str());
            }

            // 在常用阻值中搜索最接近目标上升时间的阻值
            double bestResistor = kCommonResistors.front();
            double minError = std::numeric_limits<double>::max();
            for (double r : kCommonResistors) {
                const double rTime = CalculateRiseTimeNs(r, input.loadCapacitancePf);
                const double err = std::fabs(rTime - clampedTarget);
                if (err < minError) {
                    minError = err;
                    bestResistor = r;
                }
            }

            const double actualRiseNs = CalculateRiseTimeNs(bestResistor, input.loadCapacitancePf);
            output.recommendedResistorOhm = bestResistor;
            output.actualRiseTimeNs = actualRiseNs;
            output.riseTimeRatioPct = (actualRiseNs / bitTimeNs) * 100.0;
            output.isSuitable = actualRiseNs <= maxRiseTimeNs;

            // 基于波特率与电缆长度推荐模式
            for (const auto& p : kSlopeProfiles) {
                const bool baudCond = (static_cast<double>(input.baudrate) <= p.maxBaudrate) || (std::string(p.mode) == "fast");
                const bool lenCond = input.cableLengthMeters >= p.minCableLength;

                if ((std::string(p.mode) == "fast" && lenCond) ||
                    (std::string(p.mode) != "fast" && baudCond && lenCond)) {
                    output.recommendedMode = p.mode;
                    output.modeResistorOhm = p.resistor;
                    output.modeRiseTimeNs = p.riseTime;
                    output.modeFallTimeNs = p.fallTime;
                    std::ostringstream os;
                    os << "根据波特率 " << input.baudrate << " bps 与线路长度 "
                        << input.cableLengthMeters << " m 选择 " << p.mode << " 模式";
                    output.modeReasoning = os.str();
                    break;
                }
            }
            if (output.recommendedMode.empty()) {
                const auto& fallback = kSlopeProfiles[2];
                output.recommendedMode = fallback.mode;
                output.modeResistorOhm = fallback.resistor;
                output.modeRiseTimeNs = fallback.riseTime;
                output.modeFallTimeNs = fallback.fallTime;
                output.modeReasoning = "未匹配到合适模式，默认使用 fast 模式";
            }

            output.calculationSuccess = true;
            output.statusMessage = "计算成功";
        }
        catch (const std::exception& e) {
            output.statusMessage = std::string("计算异常: ") + e.what();
        }

        return output;
    }

    // ----------------------------------------------------------------------------
    // 报文 ID 分配实现
    // ----------------------------------------------------------------------------
    namespace {
        static std::string ToBinary(uint32_t idValue, bool useExtendedId) {
            const uint32_t width = useExtendedId ? 29u : 11u;
            std::string binary(width, '0');
            uint32_t temp = idValue;
            for (int i = static_cast<int>(width) - 1; i >= 0; --i) {
                binary[i] = (temp & 1u) ? '1' : '0';
                temp >>= 1;
            }
            return binary;
        }

        static uint32_t GetMaxId(bool useExtendedId) {
            return useExtendedId ? 0x1FFFFFFFu : 0x7FFu;
        }

        static uint32_t ClampId(uint32_t candidate, bool useExtendedId) {
            return std::min(candidate, GetMaxId(useExtendedId));
        }
    } // namespace

    MessageIdAllocationOutput AllocateMessageIds(const MessageIdAllocationInput& input) {
        MessageIdAllocationOutput output;
        output.useExtendedId = input.useExtendedId;

        try {
            if (input.messages.empty()) {
                output.statusMessage = "报文列表为空";
                return output;
            }

            // 按优先级（优先级数值越小优先）排序
            std::vector<std::pair<std::string, uint32_t>> sorted = input.messages;
            std::sort(sorted.begin(), sorted.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; });

            std::set<uint32_t> usedIds;
            uint32_t minAssigned = GetMaxId(input.useExtendedId);
            uint32_t maxAssigned = 0;

            for (const auto& pr : sorted) {
                const std::string name = pr.first;
                const uint32_t priority = pr.second;

                if (name.empty()) {
                    output.warnings.push_back("发现空名称报文，已跳过");
                    continue;
                }
                if (output.allocatedIds.find(name) != output.allocatedIds.end()) {
                    output.warnings.push_back("报文 " + name + " 已分配过 ID，跳过");
                    continue;
                }

                uint32_t candidate = ClampId(priority, input.useExtendedId);

                // 若 candidate 被占用则递增查找空位（到最大 ID 后停止）
                uint32_t maxId = GetMaxId(input.useExtendedId);
                while (usedIds.count(candidate) != 0) {
                    if (candidate == maxId) {
                        // 若已满则无法分配
                        output.errors.push_back("报文 " + name + " 无法分配 ID，ID 空间可能已满");
                        break;
                    }
                    candidate = ClampId(candidate + 1u, input.useExtendedId);
                }

                if (usedIds.count(candidate) == 0) {
                    output.allocatedIds[name] = candidate;
                    output.idBinary[name] = ToBinary(candidate, input.useExtendedId);
                    usedIds.insert(candidate);
                    if (candidate < minAssigned) minAssigned = candidate;
                    if (candidate > maxAssigned) maxAssigned = candidate;
                }
            }

            output.totalMessages = static_cast<uint32_t>(output.allocatedIds.size());
            if (!output.allocatedIds.empty()) {
                output.minId = minAssigned;
                output.maxId = maxAssigned;
                output.allocationSuccess = true;
                std::ostringstream os;
                os << "成功分配 " << output.totalMessages << " 个 ID";
                output.statusMessage = os.str();
            }
            else {
                output.statusMessage = "未分配任何 ID";
            }
        }
        catch (const std::exception& e) {
            output.statusMessage = std::string("分配异常: ") + e.what();
            output.errors.push_back(e.what());
        }

        return output;
    }

    // ----------------------------------------------------------------------------
    // 报文滤波器设计实现
    // ----------------------------------------------------------------------------
    namespace {
        static uint32_t MaxId(bool useExtendedId) {
            return useExtendedId ? 0x1FFFFFFFu : 0x7FFu;
        }

        // 为给定范围计算掩码（尝试用最小位掩盖范围）
        static uint32_t MaskForRange(uint32_t minId, uint32_t maxId, bool useExtendedId) {
            if (minId > maxId) std::swap(minId, maxId);
            uint32_t range = maxId - minId;
            // 计算需要遮蔽的低位数
            uint32_t maskBits = 0;
            while ((maskBits < 31u) && ((1u << maskBits) <= range)) {
                ++maskBits;
            }
            const uint32_t baseMask = MaxId(useExtendedId);
            if (maskBits == 0) return baseMask;
            // 高位保持，低 maskBits 位为 0
            uint32_t mask = baseMask & (~((1u << maskBits) - 1u));
            return mask;
        }

        // 根据一组 ID 计算共同位掩码与过滤 ID
        static std::pair<uint32_t, uint32_t> MaskForPattern(const std::vector<uint32_t>& ids, bool useExtendedId) {
            if (ids.empty()) return { 0, 0 };
            uint32_t common = ids.front();
            uint32_t varying = 0;
            for (uint32_t v : ids) {
                common &= v;
                varying |= (ids.front() ^ v);
            }
            uint32_t mask = MaxId(useExtendedId) & (~varying);
            return { mask, ids.front() & mask };
        }
    } // namespace

    FilterDesignOutput DesignMessageFilter(const FilterDesignInput& input) {
        FilterDesignOutput output;
        output.useExtendedId = input.useExtendedId;
        output.mode = input.mode;

        try {
            if (input.acceptedIds.empty()) {
                output.statusMessage = "ID 列表不能为空";
                output.errors.push_back("接受的 ID 列表为空");
                return output;
            }

            const uint32_t maxId = MaxId(input.useExtendedId);

            switch (input.mode) {
            case FilterMode::kList: {
                for (uint32_t id : input.acceptedIds) {
                    if (id > maxId) {
                        std::ostringstream os;
                        os << "ID 0x" << std::hex << id << std::dec << " 超出范围，已跳过";
                        output.warnings.push_back(os.str());
                        continue;
                    }
                    FilterEntry ent;
                    ent.filterId = id;
                    ent.mask = maxId; // 全匹配
                    ent.entryMode = "exact";
                    ent.acceptedIds = { id };
                    output.entries.push_back(ent);
                }
                output.note = "列表模式：逐个精确匹配 ID";
                break;
            }

            case FilterMode::kRange: {
                uint32_t minId = *std::min_element(input.acceptedIds.begin(), input.acceptedIds.end());
                uint32_t maxIdVal = *std::max_element(input.acceptedIds.begin(), input.acceptedIds.end());
                uint32_t mask = MaskForRange(minId, maxIdVal, input.useExtendedId);

                FilterEntry ent;
                ent.filterId = minId;
                ent.mask = mask;
                ent.entryMode = "range";
                ent.minId = minId;
                ent.maxId = maxIdVal;
                ent.acceptedIds = input.acceptedIds;
                output.entries.push_back(ent);
                output.note = "范围模式：接受给定范围内所有 ID";
                break;
            }

            case FilterMode::kMask: {
                auto [mask, filterId] = MaskForPattern(input.acceptedIds, input.useExtendedId);
                FilterEntry ent;
                ent.filterId = filterId;
                ent.mask = mask;
                ent.entryMode = "mask";
                ent.acceptedIds = input.acceptedIds;
                output.entries.push_back(ent);
                output.note = "掩码模式：基于共同位模式匹配";
                break;
            }
            } // switch

            output.filterCount = static_cast<uint32_t>(output.entries.size());
            output.totalAcceptedIds = static_cast<uint32_t>(input.acceptedIds.size());
            output.designSuccess = !output.entries.empty();
            output.statusMessage = output.designSuccess ? "设计成功" : "设计失败";
        }
        catch (const std::exception& e) {
            output.statusMessage = std::string("设计异常: ") + e.what();
            output.errors.push_back(e.what());
        }

        return output;
    }

    // ----------------------------------------------------------------------------
    // 辅助验证函数（简单封装）
    // ----------------------------------------------------------------------------
    bool ValidateSlopeControl(const SlopeControlOutput& output) {
        return output.calculationSuccess && output.isSuitable;
    }

    bool ValidateIdAllocation(const MessageIdAllocationOutput& output) {
        return output.allocationSuccess && output.errors.empty();
    }

    bool ValidateFilterDesign(const FilterDesignOutput& output) {
        return output.designSuccess && output.errors.empty();
    }

    // ----------------------------------------------------------------------------
    // 报告生成函数（供 GUI 直接显示多行文本）
    // ----------------------------------------------------------------------------
    std::string GenerateSlopeControlReport(const SlopeControlOutput& output) {
        std::ostringstream oss;
        oss << "斜率控制电阻推荐报告\n";
        oss << "=====================\n";
        oss << "推荐电阻: " << output.recommendedResistorOhm << " Ω\n";
        oss << "实际上升时间: " << output.actualRiseTimeNs << " ns\n";
        oss << "目标上升时间(已限): " << output.targetRiseTimeNs << " ns\n";
        oss << "位时间: " << output.bitTimeNs << " ns\n";
        oss << "上升时间占比: " << output.riseTimeRatioPct << " %\n";
        oss << "是否满足时序: " << (output.isSuitable ? "是" : "否") << "\n";
        oss << "推荐模式: " << output.recommendedMode << "\n";
        oss << "模式电阻: " << output.modeResistorOhm << " Ω\n";
        oss << "模式说明: " << output.modeReasoning << "\n";
        if (!output.warnings.empty()) {
            oss << "警告:\n";
            for (const auto& w : output.warnings) oss << "  - " << w << "\n";
        }
        oss << "状态: " << output.statusMessage << "\n";
        return oss.str();
    }

    std::string GenerateIdAllocationReport(const MessageIdAllocationOutput& output) {
        std::ostringstream oss;
        oss << "报文 ID 分配报告\n";
        oss << "=================\n";
        oss << "总报文数: " << output.totalMessages << "\n";
        if (output.totalMessages > 0) {
            oss << "ID 范围: 0x" << std::hex << output.minId << " - 0x" << output.maxId << std::dec << "\n";
        }
        oss << "分配详情:\n";
        for (const auto& kv : output.allocatedIds) {
            oss << "  " << kv.first << ": 0x" << std::hex << kv.second << std::dec;
            auto it = output.idBinary.find(kv.first);
            if (it != output.idBinary.end()) {
                oss << " (" << it->second << ")";
            }
            oss << "\n";
        }
        if (!output.errors.empty()) {
            oss << "错误:\n";
            for (const auto& e : output.errors) oss << "  - " << e << "\n";
        }
        if (!output.warnings.empty()) {
            oss << "警告:\n";
            for (const auto& w : output.warnings) oss << "  - " << w << "\n";
        }
        oss << "状态: " << output.statusMessage << "\n";
        return oss.str();
    }

    std::string GenerateFilterDesignReport(const FilterDesignOutput& output) {
        std::ostringstream oss;
        oss << "报文滤波器设计报告\n";
        oss << "===================\n";
        oss << "模式: ";
        switch (output.mode) {
        case FilterMode::kList: oss << "列表模式"; break;
        case FilterMode::kRange: oss << "范围模式"; break;
        case FilterMode::kMask: oss << "掩码模式"; break;
        }
        oss << "\n";
        oss << "滤波器数量: " << output.filterCount << "\n";
        oss << "接受的 ID 总数: " << output.totalAcceptedIds << "\n";
        oss << "说明: " << output.note << "\n";
        for (size_t i = 0; i < output.entries.size(); ++i) {
            const auto& e = output.entries[i];
            oss << "  滤波器 " << (i + 1) << ":\n";
            oss << "    filterId: 0x" << std::hex << e.filterId << std::dec << "\n";
            oss << "    mask: 0x" << std::hex << e.mask << std::dec << "\n";
            oss << "    mode: " << e.entryMode << "\n";
            if (e.minId.has_value() && e.maxId.has_value()) {
                oss << "    范围: 0x" << std::hex << e.minId.value()
                    << " - 0x" << e.maxId.value() << std::dec << "\n";
            }
        }
        if (!output.errors.empty()) {
            oss << "错误:\n";
            for (const auto& e : output.errors) oss << "  - " << e << "\n";
        }
        if (!output.warnings.empty()) {
            oss << "警告:\n";
            for (const auto& w : output.warnings) oss << "  - " << w << "\n";
        }
        oss << "状态: " << output.statusMessage << "\n";
        return oss.str();
    }

    // ----------------------------------------------------------------------------
    // 统一入口：CalculateAll
    // ----------------------------------------------------------------------------
    Canopt1Output CalculateAll(const Canopt1Input& input) {
        Canopt1Output out;
        out.success = true; // 假设成功，若某项失败将设置为 false 并记录错误

        // 斜率控制
        if (input.enableSlope) {
            out.slopeExecuted = true;
            out.slope = CalculateSlopeControl(input.slope);
            // 收集警告/错误
            for (const auto& w : out.slope.warnings) out.warnings.push_back("slope: " + w);
            if (!out.slope.calculationSuccess) {
                out.errors.push_back("slope: " + out.slope.statusMessage);
                out.success = false;
            }
            else {
                // 若不满足时序也视为 warning
                if (!out.slope.isSuitable) {
                    std::ostringstream os;
                    os << "slope: 推荐电阻导致上升时间 " << out.slope.actualRiseTimeNs << " ns 超过允许值";
                    out.warnings.push_back(os.str());
                }
            }
        }

        // ID 分配
        if (input.enableIdAllocation) {
            out.idAllocationExecuted = true;
            out.idAlloc = AllocateMessageIds(input.idAlloc);
            for (const auto& w : out.idAlloc.warnings) out.warnings.push_back("idAlloc: " + w);
            for (const auto& e : out.idAlloc.errors) out.errors.push_back("idAlloc: " + e);
            if (!out.idAlloc.allocationSuccess) {
                // 仅当 allocationSuccess 为 false 且存在错误时认为失败
                if (!out.idAlloc.errors.empty()) {
                    out.success = false;
                }
            }
        }

        // 滤波器设计
        if (input.enableFilter) {
            out.filterExecuted = true;
            out.filter = DesignMessageFilter(input.filter);
            for (const auto& w : out.filter.warnings) out.warnings.push_back("filter: " + w);
            for (const auto& e : out.filter.errors) out.errors.push_back("filter: " + e);
            if (!out.filter.designSuccess) {
                if (!out.filter.errors.empty()) out.success = false;
            }
        }

        // 汇总状态信息
        std::ostringstream status;
        status << "CalculateAll: ";
        if (out.success) status << "全部执行成功";
        else status << "存在错误或警告，请检查 out.errors / out.warnings";
        out.statusMessage = status.str();

        return out;
    }

} // namespace canopt1
