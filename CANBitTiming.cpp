/**
 * @file CANBitTiming.cpp
 * @brief CAN位时序参数计算模块 - 实现文件
 *
 * 本文件实现函数式接口的所有具体逻辑
 * 提供简单易用的函数调用方式，便于其他同学整合
 */

#include "CANBitTiming.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace canopt {

    // 内部常量和辅助函数
    namespace {

        /**
         * @brief CAN位时序参数约束定义
         */
        constexpr struct {
            uint32_t minBRP{ 1 };
            uint32_t maxBRP{ 1024 };
            uint32_t minSJW{ 1 };
            uint32_t maxSJW{ 4 };
            uint32_t minTSEG1{ 1 };
            uint32_t maxTSEG1{ 16 };
            uint32_t minTSEG2{ 1 };
            uint32_t maxTSEG2{ 8 };
            uint32_t minTotalTQ{ 8 };
            uint32_t maxTotalTQ{ 25 };
        } kBitTimingConstraints;

        /**
         * @brief 标准CAN波特率定义
         */
        constexpr uint32_t kStandardBaudRates[] = {
            10000,   20000,   50000,   100000,  125000,  250000,
            500000,  800000,  1000000
        };

        /**
         * @brief 内部计算函数：寻找最优BRP和总时间份额
         */
        std::pair<uint32_t, uint32_t> FindOptimalBRPAndTQ(
            uint32_t systemClock, double targetBitTime) {

            double minError = std::numeric_limits<double>::max();
            uint32_t bestBRP = kBitTimingConstraints.minBRP;
            uint32_t bestTotalTQ = 0;

            for (uint32_t brp = kBitTimingConstraints.minBRP;
                brp <= kBitTimingConstraints.maxBRP; brp++) {

                double timeQuanta = (brp + 1) * 1.0 / systemClock;

                for (uint32_t totalTQ = kBitTimingConstraints.minTotalTQ;
                    totalTQ <= kBitTimingConstraints.maxTotalTQ; totalTQ++) {

                    double calculatedBitTime = totalTQ * timeQuanta;
                    double error = std::abs(calculatedBitTime - targetBitTime) / targetBitTime * 100.0;

                    if (error < minError) {
                        minError = error;
                        bestBRP = brp;
                        bestTotalTQ = totalTQ;
                    }

                    if (minError < 0.1) {
                        return { bestBRP, bestTotalTQ };
                    }
                }
            }

            return { bestBRP, bestTotalTQ };
        }

        /**
         * @brief 内部计算函数：分配时间段参数
         */
        bool AllocateTimeSegments(uint32_t totalTQ, uint32_t& TSEG1, uint32_t& TSEG2) {
            uint32_t remainingTQ = totalTQ - 1; // SYNC_SEG固定为1

            if (remainingTQ < 3) return false;

            // 经验分配：PROP_SEG 30%, PHASE_SEG1 35%, PHASE_SEG2 35%
            uint32_t propSeg = std::max(1U, static_cast<uint32_t>(remainingTQ * 0.3));
            propSeg = std::min(propSeg, 8U);

            uint32_t phaseRemaining = remainingTQ - propSeg;
            uint32_t phaseSeg1 = std::max(1U, phaseRemaining / 2);
            uint32_t phaseSeg2 = phaseRemaining - phaseSeg1;

            // 调整确保PHASE_SEG2 ≥ 2
            if (phaseSeg2 < 2) {
                phaseSeg2 = 2;
                phaseSeg1 = phaseRemaining - phaseSeg2;
                if (phaseSeg1 < 1) return false;
            }

            phaseSeg1 = std::min(phaseSeg1, 8U);
            phaseSeg2 = std::min(phaseSeg2, 8U);

            TSEG1 = propSeg + phaseSeg1 - 1;
            TSEG2 = phaseSeg2 - 1;

            return (TSEG1 >= (kBitTimingConstraints.minTSEG1 - 1) &&
                TSEG1 <= (kBitTimingConstraints.maxTSEG1 - 1) &&
                TSEG2 >= (kBitTimingConstraints.minTSEG2 - 1) &&
                TSEG2 <= (kBitTimingConstraints.maxTSEG2 - 1));
        }

        /**
         * @brief 内部计算函数：计算同步跳转宽度
         */
        uint32_t CalculateSJW(uint32_t TSEG1, uint32_t TSEG2) {
            uint32_t phaseSeg1 = (TSEG1 + 1) / 2;
            uint32_t phaseSeg2 = TSEG2 + 1;

            uint32_t sjw = std::min(phaseSeg1, phaseSeg2);
            sjw = std::min(sjw, kBitTimingConstraints.maxSJW);
            sjw = std::max(sjw, kBitTimingConstraints.minSJW);

            return sjw - 1;
        }

        /**
         * @brief 内部计算函数：生成BTR寄存器值
         */
        uint32_t GenerateBTRValue(uint32_t BRP, uint32_t SJW, uint32_t TSEG1, uint32_t TSEG2) {
            uint32_t btr = 0;
            btr |= (SJW & 0x03) << 24;
            btr |= (TSEG2 & 0x07) << 20;
            btr |= (TSEG1 & 0x0F) << 16;
            btr |= (BRP & 0x3FF);
            return btr;
        }

    } // namespace

    // 公共函数实现
    BitTimingOutput CalculateBitTiming(const BitTimingInput& input) {
        BitTimingOutput output;

        // 输入验证
        if (input.systemClock == 0) {
            output.calculationSuccess = false;
            output.statusMessage = "错误：系统时钟频率不能为0";
            return output;
        }

        if (input.targetBaudRate == 0 || input.targetBaudRate > 1000000) {
            output.calculationSuccess = false;
            output.statusMessage = "错误：目标波特率超出有效范围(0-1Mbps)";
            return output;
        }

        try {
            // 计算目标位时间
            double targetBitTime = 1.0 / static_cast<double>(input.targetBaudRate);

            // 寻找最优BRP和总时间份额
            auto [bestBRP, totalTQ] = FindOptimalBRPAndTQ(input.systemClock, targetBitTime);

            // 检查总时间份额有效性
            if (totalTQ < kBitTimingConstraints.minTotalTQ ||
                totalTQ > kBitTimingConstraints.maxTotalTQ) {
                output.calculationSuccess = false;
                output.statusMessage = "错误：无法找到合适的位时序参数";
                return output;
            }

            // 分配时间段参数
            uint32_t TSEG1, TSEG2;
            if (!AllocateTimeSegments(totalTQ, TSEG1, TSEG2)) {
                output.calculationSuccess = false;
                output.statusMessage = "错误：时间段分配失败";
                return output;
            }

            // 计算SJW
            uint32_t SJW = CalculateSJW(TSEG1, TSEG2);

            // 计算实际波特率和误差
            double timeQuanta = (bestBRP + 1) * 1.0 / input.systemClock;
            double actualBitTime = totalTQ * timeQuanta;
            uint32_t actualBaudRate = static_cast<uint32_t>(1.0 / actualBitTime);
            double errorPercent = std::abs(static_cast<double>(actualBaudRate) -
                input.targetBaudRate) / input.targetBaudRate * 100.0;

            // 填充输出结果
            output.BRP = bestBRP;
            output.SJW = SJW;
            output.TSEG1 = TSEG1;
            output.TSEG2 = TSEG2;
            output.totalTimeQuanta = totalTQ;
            output.actualBaudRate = actualBaudRate;
            output.errorPercent = errorPercent;
            output.btrRegister = GenerateBTRValue(bestBRP, SJW, TSEG1, TSEG2);
            output.calculationSuccess = true;
            output.statusMessage = "计算成功完成";

            // 检查误差警告
            if (errorPercent > input.maxErrorPercent) {
                output.warnings.push_back(
                    "波特率误差" + std::to_string(errorPercent) +
                    "%超过设定阈值" + std::to_string(input.maxErrorPercent) + "%");
            }

        }
        catch (const std::exception& e) {
            output.calculationSuccess = false;
            output.statusMessage = std::string("计算过程中发生错误：") + e.what();
        }

        return output;
    }

    BitTimingValidation ValidateBitTiming(const BitTimingOutput& output) {
        BitTimingValidation validation;
        validation.isValid = true;
        validation.meetsSpecification = true;

        // 验证BRP范围
        if (output.BRP < kBitTimingConstraints.minBRP ||
            output.BRP > kBitTimingConstraints.maxBRP) {
            validation.isValid = false;
            validation.errors.push_back("BRP值超出有效范围");
        }

        // 验证SJW范围
        if (output.SJW < (kBitTimingConstraints.minSJW - 1) ||
            output.SJW >(kBitTimingConstraints.maxSJW - 1)) {
            validation.isValid = false;
            validation.errors.push_back("SJW值超出有效范围");
        }

        // 验证TSEG1范围
        if (output.TSEG1 < (kBitTimingConstraints.minTSEG1 - 1) ||
            output.TSEG1 >(kBitTimingConstraints.maxTSEG1 - 1)) {
            validation.isValid = false;
            validation.errors.push_back("TSEG1值超出有效范围");
        }

        // 验证TSEG2范围
        if (output.TSEG2 < (kBitTimingConstraints.minTSEG2 - 1) ||
            output.TSEG2 >(kBitTimingConstraints.maxTSEG2 - 1)) {
            validation.isValid = false;
            validation.errors.push_back("TSEG2值超出有效范围");
        }

        // 验证总时间份额
        uint32_t calculatedTotalTQ = 1 + (output.TSEG1 + 1) + (output.TSEG2 + 1);
        if (calculatedTotalTQ < kBitTimingConstraints.minTotalTQ ||
            calculatedTotalTQ > kBitTimingConstraints.maxTotalTQ) {
            validation.isValid = false;
            validation.errors.push_back("总时间份额超出有效范围");
        }

        // 验证时间段关系
        if (output.TSEG2 + 1 <= output.SJW + 1) {
            validation.isValid = false;
            validation.errors.push_back("TSEG2必须大于SJW");
        }

        // 生成验证总结
        if (validation.isValid) {
            validation.summary = "参数验证通过，符合CAN 2.0规范";
        }
        else {
            validation.summary = "参数验证失败，存在" +
                std::to_string(validation.errors.size()) + "个错误";
        }

        return validation;
    }

    std::vector<BitTimingOutput> CalculateMultipleBaudRates(
        uint32_t systemClock,
        const std::vector<uint32_t>& baudRates) {

        std::vector<BitTimingOutput> results;

        for (uint32_t baudRate : baudRates) {
            BitTimingInput input;
            input.systemClock = systemClock;
            input.targetBaudRate = baudRate;

            results.push_back(CalculateBitTiming(input));
        }

        // 按误差排序，便于选择最优配置
        std::sort(results.begin(), results.end(),
            [](const BitTimingOutput& a, const BitTimingOutput& b) {
                return a.errorPercent < b.errorPercent;
            });

        return results;
    }

    std::vector<uint32_t> GetStandardCanBaudRates() {
        return std::vector<uint32_t>(std::begin(kStandardBaudRates),
            std::end(kStandardBaudRates));
    }

    std::string GenerateTimingReport(const BitTimingOutput& output) {
        std::ostringstream report;

        report << "=== CAN位时序参数报告 ===" << std::endl;
        report << "状态: " << output.statusMessage << std::endl;

        if (output.calculationSuccess) {
            report << std::endl << "核心参数:" << std::endl;
            report << "BRP: " << output.BRP << std::endl;
            report << "SJW: " << output.SJW << std::endl;
            report << "TSEG1: " << output.TSEG1 << std::endl;
            report << "TSEG2: " << output.TSEG2 << std::endl;
            report << "总时间份额: " << output.totalTimeQuanta << std::endl;

            report << std::endl << "精度评估:" << std::endl;
            report << "目标波特率: " << output.actualBaudRate << " bps" << std::endl;
            report << "误差百分比: " << output.errorPercent << "%" << std::endl;

            report << std::endl << "硬件配置:" << std::endl;
            report << "CAN_BTR寄存器: 0x" << std::hex << output.btrRegister << std::dec << std::endl;

            if (!output.warnings.empty()) {
                report << std::endl << "警告信息:" << std::endl;
                for (const auto& warning : output.warnings) {
                    report << "- " << warning << std::endl;
                }
            }
        }

        return report.str();
    }

    std::pair<bool, std::string> CheckCompatibility(
        uint32_t systemClock,
        uint32_t targetBaudRate) {

        if (systemClock == 0) {
            return { false, "系统时钟频率不能为0" };
        }

        if (targetBaudRate == 0) {
            return { false, "目标波特率不能为0" };
        }

        if (targetBaudRate > 1000000) {
            return { false, "目标波特率超出CAN总线支持范围(>1Mbps)" };
        }

        // 粗略的兼容性检查
        double minTimeQuanta = (1 + 1) * 1.0 / systemClock; // 最小BRP=1
        double maxBitTime = kBitTimingConstraints.maxTotalTQ * minTimeQuanta;
        double minBaudRate = 1.0 / maxBitTime;

        if (targetBaudRate < minBaudRate) {
            return { false, "目标波特率过低，当前系统时钟无法支持" };
        }

        return { true, "系统时钟和目标波特率兼容" };
    }

} // namespace canopt


