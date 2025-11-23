/**
 * @file OptionalCanFeatures.hpp
 * @brief CAN网络可选功能模块 - 头文件
 *
 * 本模块实现三个核心功能：
 * 1. 收发器斜率控制电阻选择 - 根据波特率和电缆长度推荐合适的斜率控制电阻
 * 2. 报文ID分配 - 按优先级自动分配CAN报文ID，避免冲突
 * 3. 报文滤波寄存器参数设计 - 设计CAN接收滤波器配置（列表/范围/掩码模式）
 *
 * 设计思路：
 * - 采用模块化设计，每个功能独立封装为类
 * - 使用结构体封装返回结果，便于Qt界面直接使用
 * - 提供OptionalFeatureService作为统一入口，方便集成
 * - 支持标准ID（11位）和扩展ID（29位）两种模式
 */
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace canopt {

    /**
     * @enum FilterMode
     * @brief 滤波器工作模式枚举
     *
     * 设计思路：CAN接收滤波器有三种常见配置模式
     * - kList: 列表模式，精确匹配指定的ID列表（每个ID单独配置）
     * - kRange: 范围模式，接受ID范围内的所有报文（节省滤波器资源）
     * - kMask: 掩码模式，通过位掩码匹配具有共同位模式的ID（灵活配置）
     */
    enum class FilterMode { kList, kRange, kMask };

    /**
 * @struct SlopeResistorResult
 * @brief 斜率控制电阻推荐结果
 *
 * 设计思路：封装电阻推荐算法的所有输出信息，便于Qt界面显示
 * - recommendedResistorOhm: 推荐的电阻值（欧姆），从常用阻值列表中选择最接近的
 * - actualRiseTimeNs: 使用推荐电阻后的实际上升时间（纳秒）
 * - targetRiseTimeNs: 目标上升时间（纳秒），可能被限制在最大允许值内
 * - bitTimeNs: CAN位时间（纳秒），用于计算上升时间占比
 * - riseTimeRatioPct: 上升时间占位时间的百分比，通常应小于10%
 * - isSuitable: 推荐的电阻是否满足时序要求
 */
    struct SlopeResistorResult {
        double recommendedResistorOhm{ 0.0 };   ///< 推荐的电阻值（欧姆）
        double actualRiseTimeNs{ 0.0 };   ///< 实际上升时间（纳秒）
        double targetRiseTimeNs{ 0.0 };   ///< 目标上升时间（纳秒）
        double bitTimeNs{ 0.0 };   ///< 位时间（纳秒）
        double riseTimeRatioPct{ 0.0 };   ///< 上升时间占比（%）
        bool isSuitable{ false };   ///< 是否满足时序要求
    };

    /**
 * @struct SlopeModeRecommendation
 * @brief 斜率模式推荐结果
 *
 * 设计思路：根据波特率和电缆长度，推荐快速/正常/慢速三种模式之一
 * - mode: 模式名称（"fast"/"normal"/"slow"）
 * - resistorOhm: 该模式对应的典型电阻值
 * - riseTimeNs/fallTimeNs: 该模式的典型上升/下降时间
 * - reasoning: 推荐理由说明，便于用户理解
 */
    struct SlopeModeRecommendation {
        std::string mode;   ///< 模式名称
        double resistorOhm{ 0.0 };   ///< 电阻值（欧姆）
        double riseTimeNs{ 0.0 };   ///< 上升时间（纳秒）
        double fallTimeNs{ 0.0 };   ///< 下降时间（纳秒）
        std::string reasoning;    ///< 推荐理由
    };

    /**
 * @struct AllocationInfo
 * @brief 报文ID分配信息
 *
 * 设计思路：记录每个报文分配到的ID及其详细信息
 * - id: 分配的CAN ID值
 * - priority: 优先级（数字越小优先级越高）
 * - idBinary: ID的二进制表示，便于调试和显示
 */
    struct AllocationInfo {
        uint32_t id{ 0 };   ///< 分配的ID值
        uint32_t priority{ 0 };   ///< 优先级
        std::string idBinary;   ///< ID的二进制字符串表示
    };

    struct FilterEntry {
        uint32_t filterId{ 0 };   ///< 滤波器ID
        uint32_t mask{ 0 };   ///< 掩码值
        std::string entryMode;   ///< 条目模式
        std::optional<uint32_t> minId;   ///< 最小ID（范围模式）
        std::optional<uint32_t> maxId;   ///< 最大ID（范围模式）
        std::vector<uint32_t> acceptedIds;   ///< 接受的ID列表
    };

    /**
 * @struct FilterConfig
 * @brief 完整的滤波器配置
 *
 * 设计思路：封装滤波器的完整配置信息
 * - mode: 滤波器工作模式
 * - entries: 所有配置条目
 * - note: 配置说明，便于用户理解
 */
    struct FilterConfig {
        FilterMode mode{ FilterMode::kList };   ///< 滤波器模式
        std::vector<FilterEntry> entries;   ///< 配置条目列表
        std::string note;   ///< 配置说明
    };

    /**
 * @struct FilterValidationResult
 * @brief 滤波器配置验证结果
 *
 * 设计思路：验证滤波器参数是否合法，返回错误信息列表
 * - valid: 配置是否有效
 * - errors: 错误信息列表（如果有）
 */
    struct FilterValidationResult {
        bool valid{ true };   ///< 是否有效
        std::vector<std::string> errors;    ///< 错误信息列表
    };

    /**
     * @class TransceiverSlopeController
     * @brief 收发器斜率控制电阻选择器
     *
     * 设计思路：
     * CAN收发器的斜率控制通过外部电阻调节上升/下降时间，影响EMI和信号完整性
     * - 较慢的斜率（大电阻）可以降低EMI，但会增加传播延迟
     * - 较快的斜率（小电阻）适合高速短距离，但EMI较大
     *
     * 核心算法：
     * 1. 根据RC时间常数计算上升时间：tr = 2.2 * R * C
     * 2. 从常用阻值列表中选择最接近目标上升时间的电阻
     * 3. 验证上升时间是否满足时序要求（通常应小于位时间的10%）
     */
    class TransceiverSlopeController {
    public:
        /**
     * @brief 推荐斜率控制电阻值
     *
     * 算法思路：
     * 1. 计算位时间和最大允许上升时间
     * 2. 如果目标上升时间超过限制，使用限制值
     * 3. 遍历常用阻值列表，计算每个阻值对应的上升时间
     * 4. 选择误差最小的阻值作为推荐值
     *
     * @param targetRiseTimeNs 目标上升时间（纳秒）
     * @param baudrate 波特率（bps）
     * @param maxRiseTimeRatio 最大上升时间与位时间的比值（默认10%）
     * @param loadCapacitancePf 负载电容（皮法，默认100pF）
     * @return SlopeResistorResult 推荐结果
     */
        SlopeResistorResult RecommendResistor(double targetRiseTimeNs,
            uint32_t baudrate,
            double maxRiseTimeRatio = 0.1,
            double loadCapacitancePf = 100.0) const;
        /**
     * @brief 根据波特率和电缆长度推荐斜率模式
     *
     * 算法思路：
     * 根据经验规则选择模式：
     * - 低速长距离（≤125kbps, >100m）→ 慢速模式（120Ω）
     * - 中速中距离（≤500kbps, >50m）→ 正常模式（47Ω）
     * - 高速短距离 → 快速模式（10Ω）
     *
     * @param baudrate 波特率（bps）
     * @param cableLengthMeters 电缆长度（米）
     * @return SlopeModeRecommendation 推荐结果
     */
        SlopeModeRecommendation RecommendMode(uint32_t baudrate,
            double cableLengthMeters) const;
        /**
     * @brief 计算给定电阻的上升时间
     *
     * 公式：tr = 2.2 * R * C
     * 其中2.2是10%-90%上升时间的系数
     *
     * @param resistanceOhm 电阻值（欧姆）
     * @param loadCapacitancePf 负载电容（皮法，默认100pF）
     * @return double 上升时间（纳秒）
     */
        double CalculateRiseTimeNs(double resistanceOhm,
            double loadCapacitancePf = 100.0) const;

    private:
        static constexpr double kRiseTimeFactor = 2.2;  // 10%~90%   ///< 上升时间系数（10%-90%）
        static const std::vector<double> kCommonResistors;   ///< 常用阻值列表（欧姆）
    };

    /**
 * @class MessageIdAllocator
 * @brief CAN报文ID分配器
 *
 * 设计思路：
 * CAN总线使用非破坏性仲裁机制，ID越小优先级越高
 * - 按优先级从低到高分配ID（优先级数字越小，分配的ID越小）
 * - 自动检测并避免ID冲突
 * - 支持标准ID（11位，0-2047）和扩展ID（29位，0-536870911）
 *
 * 核心算法：
 * 1. 使用优先级作为初始候选ID
 * 2. 如果ID已被占用，自动递增直到找到可用ID
 * 3. 使用set记录已分配的ID，O(log n)查找效率
 * 4. 使用map存储报文名称到分配信息的映射
 */
    class MessageIdAllocator {
    public:
        /**
     * @brief 构造函数
     * @param useExtendedId 是否使用扩展ID（29位），默认false（11位标准ID）
     */
        explicit MessageIdAllocator(bool useExtendedId = false);
        /**
     * @brief 为报文分配ID
     *
     * 算法思路：
     * 1. 如果报文已分配过ID，直接返回
     * 2. 使用优先级作为候选ID（优先级越高，ID越小）
     * 3. 如果候选ID已被占用，递增直到找到可用ID
     * 4. 记录分配信息并返回
     *
     * @param messageName 报文名称（唯一标识）
     * @param priority 优先级（0为最高优先级，数字越大优先级越低）
     * @return uint32_t 分配的ID值
     * @throws std::invalid_argument 如果报文名称为空
     */
        uint32_t Allocate(const std::string& messageName, uint32_t priority);
        /**
     * @brief 获取所有已分配的ID信息
     * @return std::map<std::string, AllocationInfo> 报文名称到分配信息的映射
     */
        std::map<std::string, AllocationInfo> GetAllocations() const;

        /**
     * @brief 重置分配器，清空所有已分配的ID
     */
        void Reset();

    private:
        /**
     * @brief 将ID限制在有效范围内
     * @param candidate 候选ID
     * @return uint32_t 限制后的ID
     */
        uint32_t ClampId(uint32_t candidate) const;

        /**
     * @brief 获取最大有效ID
     * @return uint32_t 标准ID返回0x7FF，扩展ID返回0x1FFFFFFF
     */
        uint32_t GetMaxId() const;

        /**
     * @brief 将ID转换为二进制字符串
     * @param idValue ID值
     * @return std::string 二进制字符串（11位或29位）
     */
        std::string ToBinary(uint32_t idValue) const;

        bool useExtendedId_{ false };///< 是否使用扩展ID
        std::map<std::string, AllocationInfo> allocations_;///< 报文名称到分配信息的映射
        std::set<uint32_t> usedIds_;///< 已使用的ID集合（用于快速查找冲突）
    };

    /**
 * @class MessageFilterDesigner
 * @brief CAN报文滤波器设计器
 *
 * 设计思路：
 * CAN接收滤波器用于过滤不需要的报文，减少CPU负载
 * - 列表模式：精确匹配指定的ID列表（每个ID单独配置，最精确但占用资源多）
 * - 范围模式：接受ID范围内的所有报文（节省滤波器资源，适合连续ID）
 * - 掩码模式：通过位掩码匹配具有共同位模式的ID（灵活配置，适合有规律的ID）
 *
 * 核心算法：
 * 1. 列表模式：为每个ID创建精确匹配的滤波器条目（掩码全1）
 * 2. 范围模式：计算覆盖整个范围的掩码（高位匹配，低位忽略）
 * 3. 掩码模式：找到所有ID的共同位模式，生成掩码
 */
    class MessageFilterDesigner {
    public:
        /**
     * @brief 构造函数
     * @param useExtendedId 是否使用扩展ID（29位），默认false（11位标准ID）
     */
        explicit MessageFilterDesigner(bool useExtendedId = false);

        /**
     * @brief 设计接受滤波器
     *
     * 算法思路：
     * - 列表模式：为每个ID创建精确匹配条目（掩码=MaxId，全匹配）
     * - 范围模式：找到最小/最大ID，计算覆盖范围的掩码
     * - 掩码模式：分析ID的共同位模式，生成掩码和滤波器ID
     *
     * @param ids 要接受的ID列表
     * @param mode 滤波器模式
     * @return FilterConfig 滤波器配置
     * @throws std::invalid_argument 如果ID列表为空或模式无效
     */
        FilterConfig DesignAcceptanceFilter(const std::vector<uint32_t>& ids,
            FilterMode mode) const;

        /**
     * @brief 设计拒绝滤波器
     *
     * 注意：拒绝滤波器通常需要在硬件上配置反向逻辑
     * 这里返回配置信息，实际实现需要根据硬件特性调整
     *
     * @param rejectedIds 要拒绝的ID列表
     * @return FilterConfig 滤波器配置
     */
        FilterConfig DesignRejectionFilter(const std::vector<uint32_t>& rejectedIds) const;

        /**
     * @brief 验证滤波器配置的有效性
     *
     * 检查项：
     * - 滤波器ID是否在有效范围内
     * - 掩码是否在有效范围内
     *
     * @param filterId 滤波器ID
     * @param mask 掩码值
     * @return FilterValidationResult 验证结果
     */
        FilterValidationResult Validate(uint32_t filterId, uint32_t mask) const;

    private:
        /**
     * @brief 获取最大有效ID
     * @return uint32_t 标准ID返回0x7FF，扩展ID返回0x1FFFFFFF
     */
        uint32_t MaxId() const;

        /**
     * @brief 计算范围模式的掩码
     *
     * 算法思路：
     * 1. 计算ID范围（maxId - minId）
     * 2. 找到需要忽略的最低位数量（maskBits）
     * 3. 生成掩码：高位匹配（1），低位忽略（0）
     *
     * 例如：范围0x100-0x1FF，需要忽略低8位，掩码=0x700
     *
     * @param minId 最小ID
     * @param maxId 最大ID
     * @return uint32_t 计算得到的掩码值
     */
        uint32_t MaskForRange(uint32_t minId, uint32_t maxId) const;

        /**
     * @brief 计算掩码模式的掩码和滤波器ID
     *
     * 算法思路：
     * 1. 计算所有ID的共同位（commonBits = AND所有ID）
     * 2. 计算变化的位（varyingBits = OR所有ID的异或）
     * 3. 掩码 = 不变的位设为1，变化的位设为0
     * 4. 滤波器ID = 第一个ID与掩码的AND结果
     *
     * @param ids ID列表
     * @return std::pair<uint32_t, uint32_t> (掩码, 滤波器ID)
     */
        std::pair<uint32_t, uint32_t> MaskForPattern(const std::vector<uint32_t>& ids) const;

        bool useExtendedId_{ false }; ///< 是否使用扩展ID
    };

    /**
 * @class OptionalFeatureService
 * @brief 可选功能服务类（统一入口）
 *
 * 设计思路：
 * 这是一个门面（Facade）模式的实现，将三个功能模块聚合在一起
 * - 提供统一的初始化接口（支持标准/扩展ID模式）
 * - 提供访问各个子模块的接口（const和非const版本）
 * - 便于Qt界面一次性创建服务，然后调用各个功能
 *
 * 使用示例：
 * @code
 * canopt::OptionalFeatureService service(false);  // 使用标准ID
 * auto result = service.SlopeController().RecommendResistor(150.0, 250000);
 * uint32_t id = service.Allocator().Allocate("Message1", 0);
 * @endcode
 */
    class OptionalFeatureService {
    public:
        /**
     * @brief 构造函数
     * @param useExtendedId 是否使用扩展ID（29位），默认false（11位标准ID）
     */
        OptionalFeatureService(bool useExtendedId = false);

        // 斜率控制器访问接口
        const TransceiverSlopeController& SlopeController() const { return slopeController_; }
        TransceiverSlopeController& SlopeController() { return slopeController_; }

        // ID分配器访问接口
        const MessageIdAllocator& Allocator() const { return idAllocator_; }
        MessageIdAllocator& Allocator() { return idAllocator_; }

        // 滤波器设计器访问接口
        const MessageFilterDesigner& FilterDesigner() const { return filterDesigner_; }
        MessageFilterDesigner& FilterDesigner() { return filterDesigner_; }

    private:
        TransceiverSlopeController slopeController_; ///< 斜率控制器实例
        MessageIdAllocator idAllocator_; ///< ID分配器实例
        MessageFilterDesigner filterDesigner_;  ///< 滤波器设计器实例
    };

}  // namespace canopt#pragma once
