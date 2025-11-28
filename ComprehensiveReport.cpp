/**
 * @file ComprehensiveReport.cpp
 * @brief 综合报告数据收集实现
 */

#include "ComprehensiveReport.h"
#include <ctime>
#include <sstream>
#include <iomanip>

namespace canproject {

void ComprehensiveReport::UpdateTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);

    std::ostringstream oss;
    oss << std::put_time(localTime, "%Y-%m-%d %H:%M:%S");
    timestamp = oss.str();
}

void ComprehensiveReport::ValidateOverallStatus() {
    allCalculationsSuccessful =
        baudRate.output.calculationSuccess &&
        network.output.overallSuccess &&
        bitTiming.output.calculationSuccess &&
        slopeControl.output.calculationSuccess;

    // 收集所有警告信息
    overallWarnings.clear();
    if (!baudRate.output.warningMessage.empty()) {
        overallWarnings.push_back("波特率计算: " + baudRate.output.warningMessage);
    }
    if (!bitTiming.output.statusMessage.empty() && !bitTiming.output.calculationSuccess) {
        overallWarnings.push_back("位时序计算: " + bitTiming.output.statusMessage);
    }
    if (!slopeControl.output.statusMessage.empty() && !slopeControl.output.calculationSuccess) {
        overallWarnings.push_back("斜率控制: " + slopeControl.output.statusMessage);
    }

    // 添加网络设计的警告
    if (!network.output.logs.empty()) {
        for (const auto& log : network.output.logs) {
            overallWarnings.push_back("网络设计: " + log);
        }
    }
}

void ComprehensiveReport::CollectAllData() {
    UpdateTimestamp();
    ValidateOverallStatus();
}

} // namespace canproject

