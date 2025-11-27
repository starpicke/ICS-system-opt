/**
 * @file CANBitTiming.hpp
 * @brief CAN位时序参数计算模块 - 头文件
 *
 * 本模块提供函数式接口的CAN位时序参数计算功能：
 * 1. 位时序参数计算 - 根据系统时钟和目标波特率计算BRP、SJW、TSEG1、TSEG2等参数
 * 2. 参数验证 - 验证生成的位时序参数是否符合CAN规范
 * 3. STM32寄存器生成 - 生成可直接写入STM32 CAN_BTR寄存器的值
 *
 * 设计思路：
 * - 提供简单的函数式接口，便于调用和整合
 * - 使用结构体封装输入输出参数，接口清晰
 * - 完整的错误处理和参数验证机制
 * - 支持批量计算和标准波特率查询
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace canopt {

    /**
     * @struct BitTimingInput
     * @brief 位时序计算输入参数结构体
     *
     * 设计思路：封装所有计算所需的输入参数
     * 便于函数调用和参数传递，提高接口的清晰度
     */
    struct BitTimingInput {
        uint32_t systemClock{ 48000000 };    ///< 系统时钟频率(Hz)，默认48MHz
        uint32_t targetBaudRate{ 500000 };   ///< 目标波特率(bps)，默认500kbps
        double maxErrorPercent{ 5.0 };       ///< 最大允许误差百分比，默认5%
    };

    /**
     * @struct BitTimingOutput
     * @brief 位时序计算输出结果结构体
     *
     * 设计思路：封装所有计算结果和状态信息
     * 包含核心参数、验证信息和寄存器配置值
     */
    struct BitTimingOutput {
        // 核心计算参数
        uint32_t BRP{ 0 };                   ///< 波特率预分频器
        uint32_t SJW{ 0 };                   ///< 同步跳转宽度
        uint32_t TSEG1{ 0 };                 ///< 时间段1
        uint32_t TSEG2{ 0 };                 ///< 时间段2
        uint32_t totalTimeQuanta{ 0 };       ///< 总时间份额数

        // 精度评估
        uint32_t actualBaudRate{ 0 };        ///< 实际计算出的波特率
        double errorPercent{ 0.0 };          ///< 与目标波特率的误差百分比

        // 硬件配置
        uint32_t btrRegister{ 0 };           ///< STM32 CAN_BTR寄存器值

        // 状态信息
        bool calculationSuccess{ false };    ///< 计算是否成功
        std::string statusMessage;         ///< 状态描述信息
        std::vector<std::string> warnings; ///< 警告信息列表
    };

    /**
     * @struct BitTimingValidation
     * @brief 位时序参数验证结果
     *
     * 设计思路：提供详细的验证信息
     * 便于调用者了解参数的有效性和潜在问题
     */
    struct BitTimingValidation {
        bool isValid{ false };                   ///< 参数是否有效
        bool meetsSpecification{ false };        ///< 是否符合CAN规范
        std::vector<std::string> errors;       ///< 错误信息列表
        std::vector<std::string> warnings;     ///< 警告信息列表
        std::string summary;                   ///< 验证总结
    };

    /**
     * @brief 计算CAN位时序参数（主函数）
     *
     * 算法思路：
     * 1. 输入参数验证和预处理
     * 2. 遍历BRP范围寻找最优解
     * 3. 分配时间段参数
     * 4. 计算同步跳转宽度
     * 5. 精度评估和结果验证
     *
     * @param input 输入参数结构体
     * @return BitTimingOutput 输出结果结构体
     *
     * 使用示例：
     * @code
     * canopt::BitTimingInput input;
     * input.systemClock = 48000000;
     * input.targetBaudRate = 500000;
     * auto result = canopt::CalculateBitTiming(input);
     * if (result.calculationSuccess) {
     *     // 使用result.btrRegister配置STM32
     * }
     * @endcode
     */
    BitTimingOutput CalculateBitTiming(const BitTimingInput& input);

    /**
     * @brief 验证位时序参数的有效性
     *
     * 验证逻辑：
     * - BRP范围检查：1-1024
     * - SJW范围检查：1-4
     * - TSEG1范围检查：1-16
     * - TSEG2范围检查：1-8
     * - 总时间份额检查：8-25
     * - 时间段关系检查：TSEG2 ≥ SJW
     *
     * @param output 要验证的位时序参数
     * @return BitTimingValidation 验证结果
     */
    BitTimingValidation ValidateBitTiming(const BitTimingOutput& output);

    /**
     * @brief 批量计算多个波特率的位时序参数
     *
     * 设计思路：便于系统设计时比较不同波特率的配置
     * 返回按误差排序的结果列表，便于选择最优配置
     *
     * @param systemClock 系统时钟频率
     * @param baudRates 目标波特率列表
     * @return std::vector<BitTimingOutput> 计算结果列表
     */
    std::vector<BitTimingOutput> CalculateMultipleBaudRates(
        uint32_t systemClock,
        const std::vector<uint32_t>& baudRates);

    /**
     * @brief 获取支持的标准CAN波特率列表
     *
     * @return std::vector<uint32_t> 标准波特率列表，包含常用工业标准值
     */
    std::vector<uint32_t> GetStandardCanBaudRates();

    /**
     * @brief 生成详细的参数报告字符串
     *
     * 设计思路：提供格式化的输出，便于显示和调试
     * 包含所有关键参数和验证信息
     *
     * @param output 位时序参数结果
     * @return std::string 格式化的报告字符串
     */
    std::string GenerateTimingReport(const BitTimingOutput& output);

    /**
     * @brief 检查系统时钟和目标波特率的兼容性
     *
     * 设计思路：在计算前进行快速兼容性检查
     * 避免不必要的计算过程
     *
     * @param systemClock 系统时钟频率
     * @param targetBaudRate 目标波特率
     * @return std::pair<bool, std::string> (是否兼容, 原因描述)
     */
    std::pair<bool, std::string> CheckCompatibility(
        uint32_t systemClock,
        uint32_t targetBaudRate);

} // namespace canopt



