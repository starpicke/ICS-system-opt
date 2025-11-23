/**
 * @file OptionalCanFeatures.cpp
 * @brief CAN网络可选功能模块 - 实现文件
 *
 * 本文件实现了头文件中声明的所有功能类的具体逻辑
 */

#include "OptionalCanFeatures.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace canopt {

    // 匿名命名空间：限制作用域，避免与其他文件冲突
    namespace {

        /**
 * @brief 斜率控制模式配置表
 *
 * 设计思路：根据实际应用经验，定义三种典型的斜率控制模式
 * - slow: 慢速模式，120Ω，上升时间300ns，适用于低速长距离（≤125kbps, >100m）
 * - normal: 正常模式，47Ω，上升时间150ns，适用于中速中距离（≤500kbps, >50m）
 * - fast: 快速模式，10Ω，上升时间50ns，适用于高速短距离
 *
 * 这些参数基于常见的CAN收发器（如MCP2551、TJA1050等）的典型特性
 */
        constexpr struct {
            const char* mode;///< 模式名称
            double resistor;///< 电阻值（欧姆）
            double riseTime; ///< 上升时间（纳秒）
            double fallTime;///< 下降时间（纳秒）
            double maxBaudrate;///< 最大适用波特率（bps）
            double minCableLength;///< 最小适用电缆长度（米）
        } kSlopeProfiles[] = {
            {"slow", 120.0, 300.0, 300.0, 125000.0, 100.0},// 慢速模式
            {"normal", 47.0, 150.0, 150.0, 500000.0, 50.0}, // 正常模式
            {"fast", 10.0, 50.0, 50.0, 1000000.0, 0.0}, // 快速模式
        };

    }  // namespace

    // 常用阻值列表：基于E24系列标准阻值，覆盖10Ω-200Ω范围
// 这些是实际电路中容易采购和使用的阻值
    const std::vector<double> TransceiverSlopeController::kCommonResistors = {
        10, 15, 20, 22, 27, 30, 33, 39, 47, 51, 56, 68, 75, 82, 100, 120, 150, 200 };

    /**
 * @brief 计算上升时间
 *
 * 实现思路：
 * 使用RC电路的一阶响应模型：tr = 2.2 * R * C
 * - 2.2是10%-90%上升时间的系数（基于一阶RC电路的指数响应）
 * - R是斜率控制电阻（欧姆）
 * - C是负载电容（法拉），包括PCB走线、收发器输入电容等
 *
 * 注意：这是一个简化模型，实际上升时间还受驱动能力、PCB布局等因素影响
 *
 * @param resistanceOhm 电阻值（欧姆）
 * @param loadCapacitancePf 负载电容（皮法）
 * @return double 上升时间（纳秒）
 */
    double TransceiverSlopeController::CalculateRiseTimeNs(double resistanceOhm,
        double loadCapacitancePf) const {
        const double capacitanceF = loadCapacitancePf * 1e-12;// 皮法转法拉
        return kRiseTimeFactor * resistanceOhm * capacitanceF * 1e9;// 秒转纳秒
    }

    /**
 * @brief 推荐斜率控制电阻值
 *
 * 实现思路：
 * 1. 参数验证：检查波特率是否有效
 * 2. 计算约束条件：
 *    - 位时间 = 1 / 波特率
 *    - 最大允许上升时间 = 位时间 × maxRiseTimeRatio（默认10%）
 *    - 如果目标上升时间超过限制，使用限制值（clampedTarget）
 * 3. 遍历常用阻值列表，使用穷举法找到最接近目标上升时间的阻值
 * 4. 计算实际结果并返回
 *
 * 算法复杂度：O(n)，n为常用阻值数量（18个）
 *
 * @param targetRiseTimeNs 目标上升时间（纳秒）
 * @param baudrate 波特率（bps）
 * @param maxRiseTimeRatio 最大上升时间与位时间的比值（默认10%）
 * @param loadCapacitancePf 负载电容（皮法，默认100pF）
 * @return SlopeResistorResult 推荐结果
 * @throws std::invalid_argument 如果波特率为0
 */
    SlopeResistorResult TransceiverSlopeController::RecommendResistor(
        double targetRiseTimeNs, uint32_t baudrate, double maxRiseTimeRatio,
        double loadCapacitancePf) const {
        // 参数验证
        if (baudrate == 0) {
            throw std::invalid_argument("波特率必须大于0");
        }

        // 计算位时间和最大允许上升时间
        const double bitTimeNs = 1e9 / static_cast<double>(baudrate);
        const double maxRiseTimeNs = bitTimeNs * maxRiseTimeRatio;
        // 如果目标上升时间超过限制，使用限制值
        const double clampedTarget = std::min(targetRiseTimeNs, maxRiseTimeNs);

        // 初始化：使用第一个阻值作为默认值
        double bestResistor = kCommonResistors.front();
        double minError = std::numeric_limits<double>::max();

        // 遍历所有常用阻值，找到误差最小的
        for (double resistor : kCommonResistors) {
            const double riseNs = CalculateRiseTimeNs(resistor, loadCapacitancePf);
            const double error = std::fabs(riseNs - clampedTarget);
            if (error < minError) {
                minError = error;
                bestResistor = resistor;
            }
        }

        // 计算使用推荐阻值后的实际上升时间
        const double actualRiseNs = CalculateRiseTimeNs(bestResistor, loadCapacitancePf);
        // 构造并返回结果
        return {
            bestResistor, // 推荐的电阻值
            actualRiseNs, // 实际上升时间
            clampedTarget,// 目标上升时间（可能被限制）
            bitTimeNs,// 位时间
            (actualRiseNs / bitTimeNs) * 100.0,// 上升时间占比（%）
            actualRiseNs <= maxRiseTimeNs, // 是否满足时序要求
        };
    }

    /**
 * @brief 根据波特率和电缆长度推荐斜率模式
 *
 * 实现思路：
 * 使用规则匹配算法，按优先级顺序检查三种模式：
 * 1. 慢速模式：波特率≤125kbps 且 电缆长度≥100m
 * 2. 正常模式：波特率≤500kbps 且 电缆长度≥50m
 * 3. 快速模式：其他情况（默认）
 *
 * 匹配逻辑：
 * - 从慢速到快速依次检查，找到第一个满足条件的模式
 * - 快速模式作为兜底方案，总是可以匹配（minCableLength=0）
 *
 * @param baudrate 波特率（bps）
 * @param cableLengthMeters 电缆长度（米）
 * @return SlopeModeRecommendation 推荐结果
 */
    SlopeModeRecommendation TransceiverSlopeController::RecommendMode(
        uint32_t baudrate, double cableLengthMeters) const {
        // 按优先级顺序检查模式配置表（从慢速到快速）
        for (const auto& profile : kSlopeProfiles) {
            // 检查波特率条件：快速模式总是满足，其他模式需要≤最大波特率
            const bool baudCondition = baudrate <= profile.maxBaudrate || profile.mode == std::string("fast");
            // 检查电缆长度条件：必须≥最小长度
            const bool lengthCondition = cableLengthMeters >= profile.minCableLength;

        // 匹配条件：
        // - 快速模式：只需满足长度条件
        // - 其他模式：需要同时满足波特率和长度条件
            if ((profile.mode == std::string("fast") && lengthCondition) ||
                (profile.mode != std::string("fast") && baudCondition && lengthCondition)) {
                return { profile.mode, profile.resistor, profile.riseTime, profile.fallTime,
                        "根据波特率与线路长度选择" + std::string(profile.mode) + "模式" };
            }
        }

        // 兜底：如果都不匹配（理论上不会发生），使用快速模式
        const auto& fallback = kSlopeProfiles[2];
        return { fallback.mode, fallback.resistor, fallback.riseTime, fallback.fallTime,
                "默认使用快速模式" };
    }

    /**
 * @brief 构造函数
 * @param useExtendedId 是否使用扩展ID（29位）
 */
    MessageIdAllocator::MessageIdAllocator(bool useExtendedId) : useExtendedId_(useExtendedId) {}

    /**
 * @brief 为报文分配ID
 *
 * 实现思路：
 * 1. 参数验证：检查报文名称是否为空
 * 2. 检查是否已分配：如果报文已分配过ID，直接返回（幂等性）
 * 3. 冲突检测与解决：
 *    - 使用优先级作为初始候选ID（优先级越高，ID越小）
 *    - 如果候选ID已被占用，递增直到找到可用ID
 *    - 使用set进行O(log n)的快速查找
 * 4. 记录分配信息：
 *    - 将分配信息存入map（报文名称→分配信息）
 *    - 将ID加入usedIds_集合
 *    - 生成二进制字符串表示
 *
 * 算法复杂度：O(log n)，n为已分配ID数量
 *
 * @param messageName 报文名称（唯一标识）
 * @param priority 优先级（0为最高优先级）
 * @return uint32_t 分配的ID值
 * @throws std::invalid_argument 如果报文名称为空
 */
    uint32_t MessageIdAllocator::Allocate(const std::string& messageName, uint32_t priority) {
        // 参数验证
        if (messageName.empty()) {
            throw std::invalid_argument("报文名称不能为空");
        }

        // 检查是否已分配过（幂等性：重复调用返回相同结果）
        auto it = allocations_.find(messageName);
        if (it != allocations_.end()) {
            return it->second.id;
        }

        // 使用优先级作为初始候选ID（优先级越高，ID越小）
        uint32_t idCandidate = ClampId(priority);

        // 冲突检测：如果ID已被占用，递增直到找到可用ID
        while (usedIds_.count(idCandidate) != 0) {
            idCandidate = ClampId(idCandidate + 1);
        }

        // 记录分配信息
        AllocationInfo info{ idCandidate, priority, ToBinary(idCandidate) };
        allocations_.emplace(messageName, info);// 存储分配信息
        usedIds_.insert(idCandidate);// 标记ID已使用
        return idCandidate;
    }

    /**
 * @brief 获取所有已分配的ID信息
 * @return std::map<std::string, AllocationInfo> 报文名称到分配信息的映射
 */
    std::map<std::string, AllocationInfo> MessageIdAllocator::GetAllocations() const {
        return allocations_;
    }

    /**
 * @brief 重置分配器，清空所有已分配的ID
 * 用于重新开始分配或测试场景
 */
    void MessageIdAllocator::Reset() {
        allocations_.clear();
        usedIds_.clear();
    }

    /**
 * @brief 将ID限制在有效范围内
 * @param candidate 候选ID
 * @return uint32_t 限制后的ID（不超过最大值）
 */
    uint32_t MessageIdAllocator::ClampId(uint32_t candidate) const {
        return std::min(candidate, GetMaxId());
    }

    /**
 * @brief 获取最大有效ID
 * @return uint32_t 标准ID返回0x7FF（2047），扩展ID返回0x1FFFFFFF（536870911）
 */
    uint32_t MessageIdAllocator::GetMaxId() const {
        return useExtendedId_ ? 0x1FFFFFFF : 0x7FF;
    }

    /**
 * @brief 将ID转换为二进制字符串
 *
 * 实现思路：
 * 1. 根据ID类型确定位数（标准11位，扩展29位）
 * 2. 从最低位开始，逐位提取并填充到字符串
 * 3. 使用位运算提高效率
 *
 * 示例：0x123 → "00000010010"（11位标准ID）
 *
 * @param idValue ID值
 * @return std::string 二进制字符串（高位在前）
 */
    std::string MessageIdAllocator::ToBinary(uint32_t idValue) const {
        const uint32_t width = useExtendedId_ ? 29u : 11u;
        std::string binary(width, '0');// 初始化为全0

        // 从最低位到最高位，逐位提取
        for (int i = width - 1; i >= 0; --i) {
            binary[i] = (idValue & 0x1) ? '1' : '0';  // 提取最低位
            idValue >>= 1; // 右移一位
        }
        return binary;
    }

    /**
 * @brief 构造函数
 * @param useExtendedId 是否使用扩展ID（29位）
 */
    MessageFilterDesigner::MessageFilterDesigner(bool useExtendedId)
        : useExtendedId_(useExtendedId) {}

    /**
 * @brief 设计接受滤波器
 *
 * 实现思路：
 * 根据不同的滤波模式，采用不同的配置策略：
 *
 * 1. 列表模式（kList）：
 *    - 为每个ID创建独立的滤波器条目
 *    - 掩码设为MaxId（全1），实现精确匹配
 *    - 优点：精确，缺点：占用滤波器资源多
 *
 * 2. 范围模式（kRange）：
 *    - 找到ID列表的最小值和最大值
 *    - 计算覆盖整个范围的掩码
 *    - 优点：节省资源，适合连续ID
 *
 * 3. 掩码模式（kMask）：
 *    - 分析所有ID的共同位模式
 *    - 生成掩码和滤波器ID
 *    - 优点：灵活，适合有规律的ID
 *
 * @param ids 要接受的ID列表
 * @param mode 滤波器模式
 * @return FilterConfig 滤波器配置
 * @throws std::invalid_argument 如果ID列表为空或模式无效
 */
    FilterConfig MessageFilterDesigner::DesignAcceptanceFilter(const std::vector<uint32_t>& ids,
        FilterMode mode) const {
        // 参数验证
        if (ids.empty()) {
            throw std::invalid_argument("ID列表不能为空");
        }

        FilterConfig config;
        config.mode = mode;

        switch (mode) {
        case FilterMode::kList: {
            // 列表模式：为每个ID创建精确匹配条目
            for (uint32_t id : ids) {
                // 掩码=MaxId表示全匹配（精确匹配）
                config.entries.push_back(FilterEntry{ id, MaxId(), "exact", std::nullopt,
                                                     std::nullopt, {id} });
            }
            config.note = "逐个精确匹配ID";
            break;
        }
        case FilterMode::kRange: {
            // 范围模式：找到最小/最大ID，计算覆盖掩码
            const uint32_t minId = *std::min_element(ids.begin(), ids.end());
            const uint32_t maxId = *std::max_element(ids.begin(), ids.end());
            const uint32_t mask = MaskForRange(minId, maxId);
            config.entries.push_back(
                FilterEntry{ minId, mask, "range", minId, maxId, ids });
            config.note = "接受给定范围内的所有ID";
            break;
        }
        case FilterMode::kMask: {
            // 掩码模式：分析ID的共同位模式
            const auto [mask, filterId] = MaskForPattern(ids);
            config.entries.push_back(
                FilterEntry{ filterId, mask, "mask", std::nullopt, std::nullopt, ids });
            config.note = "通过掩码匹配具有共同位模式的ID";
            break;
        }
        default:
            throw std::invalid_argument("未知滤波模式");
        }

        return config;
    }

    FilterConfig MessageFilterDesigner::DesignRejectionFilter(
        const std::vector<uint32_t>& rejectedIds) const {
        FilterConfig config;
        config.mode = FilterMode::kList;
        config.entries.push_back(
            FilterEntry{ 0, 0, "rejection", std::nullopt, std::nullopt, rejectedIds });
        config.note = "需要在硬件上配置反向逻辑，拒绝列出的ID";
        return config;
    }

    /**
 * @brief 设计拒绝滤波器
 *
 * 注意：拒绝滤波器的实现依赖于硬件特性
 * 大多数CAN控制器不直接支持拒绝模式，需要通过反向逻辑实现
 * 这里返回配置信息，实际硬件配置需要根据具体控制器调整
 *
 * @param rejectedIds 要拒绝的ID列表
 * @return FilterConfig 滤波器配置（包含说明信息）
 */
    FilterValidationResult MessageFilterDesigner::Validate(uint32_t filterId,
        uint32_t mask) const {
        FilterValidationResult result;
        const uint32_t maxId = MaxId();
        if (filterId > maxId) {
            result.valid = false;
            result.errors.emplace_back("滤波器ID超出范围");
        }
        if (mask > maxId) {
            result.valid = false;
            result.errors.emplace_back("掩码超出范围");
        }
        return result;
    }

    uint32_t MessageFilterDesigner::MaxId() const {
        return useExtendedId_ ? 0x1FFFFFFF : 0x7FF;
    }

    uint32_t MessageFilterDesigner::MaskForRange(uint32_t minId, uint32_t maxId) const {
        if (minId > maxId) {
            std::swap(minId, maxId);
        }
        uint32_t range = maxId - minId;
        uint32_t maskBits = 0;
        while ((1u << maskBits) <= range && maskBits < 32) {
            ++maskBits;
        }
        const uint32_t baseMask = MaxId();
        const uint32_t mask = baseMask & (~((1u << maskBits) - 1u));
        return mask;
    }

    std::pair<uint32_t, uint32_t> MessageFilterDesigner::MaskForPattern(
        const std::vector<uint32_t>& ids) const {
        uint32_t commonBits = ids.front();
        uint32_t varyingBits = 0;
        for (uint32_t id : ids) {
            commonBits &= id;
            varyingBits |= (ids.front() ^ id);
        }
        const uint32_t mask = MaxId() & (~varyingBits);
        return { mask, ids.front() & mask };
    }

    OptionalFeatureService::OptionalFeatureService(bool useExtendedId)
        : slopeController_(),
        idAllocator_(useExtendedId),
        filterDesigner_(useExtendedId) {}

}  // namespace canopt
//# 可选功能 C++ 模块说明
//为便于 Qt 同学集成可选功能，本说明概述 `optional_features_cpp` 目录下的核心接口与典型调用流程。
//## 模块结构
//```
//optional_features_cpp /
//├── OptionalCanFeatures.hpp   # 统一对外头文件
//└── OptionalCanFeatures.cpp   # 具体实现
//```
//所有类型均放置在 `canopt` 命名空间中，可直接被 Qt / C++ 项目引用。
//## 主要类与职责
//- `TransceiverSlopeController`
//- `SlopeResistorResult RecommendResistor(...)`
//- `SlopeModeRecommendation RecommendMode(...)`
//- `double CalculateRiseTimeNs(...)`
//- `MessageIdAllocator`
//- `uint32_t Allocate(messageName, priority)`
//- `std::map<std::string, AllocationInfo> GetAllocations()`
//- `void Reset()`
//- `MessageFilterDesigner`
//- `FilterConfig DesignAcceptanceFilter(ids, FilterMode)`
//- `FilterConfig DesignRejectionFilter(rejectedIds)`
//- `FilterValidationResult Validate(filterId, mask)`
//- `OptionalFeatureService`
//- 聚合上述三个模块，方便通过单一入口在 Qt 中注入依赖。
//## Qt 集成建议
//1. 在 Qt 项目中新增静态库 / 子模块，包含 `optional_features_cpp` 源码。
//2. 在 GUI 层创建 `OptionalFeatureService` 实例，根据界面输入调用对应方法。
//3. 使用 `FilterConfig`、`SlopeResistorResult` 等结构体直接驱动界面显示或导出配置。
//4. 若需序列化，可将结构体转为 `QVariantMap` 或自定义 DTO，再绑定到 QML / Widget。
//## 示例（伪代码）
//```cpp
//#include "optional_features_cpp/OptionalCanFeatures.hpp"
//canopt::OptionalFeatureService service(/*useExtendedId=*/true);
//auto slopeResult = service.SlopeController().RecommendResistor(
//    /*targetRiseTimeNs=*/180.0,
//    /*baudrate=*/250000,
//    /*maxRiseTimeRatio=*/0.15);
//auto id = service.Allocator().Allocate("FeedbackFrame", /*priority=*/3);
//std::vector<uint32_t> ids = { 0x100, 0x101, 0x102 };
//auto filterCfg = service.FilterDesigner().DesignAcceptanceFilter(
//    ids, canopt::FilterMode::kRange);
//```
//该模块与 Qt 解耦，可独立单元测试后再接入 GUI。