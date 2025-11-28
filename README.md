# QoptimalICS

**QoptimalICS** - 现代工业控制系统参数优化软件

> 现代工业控制网络课程研究性教学作品 | 基于Qt框架的CAN总线网络综合优化解决方案

## 🌟 项目简介

QoptimalICS是一款专业的工业控制系统参数优化软件，为CAN总线网络提供完整的设计、优化和分析工具链。本软件采用模块化架构，集成了网络拓扑设计、波特率计算、CAN ID分配等核心功能，主要使用QT实现。

## 核心功能

### 📊 波特率计算器
- **智能优化**自动计算网络负载，优化信号配置
- **精准计算**最优波特率配置，提供计算波特率，推荐波特率等多种选择

### 🛠️ 网络设计器
- **网络可视化**配置节点信息，自动绘制网络结构图
- **智能分段**根据网络负载与波特率自动划分网段

### 🔢 CAN ID分配
- **自动分配**标识符和过滤器配置
- **冲突检测**确保网络稳定性

### 📑 报告生成器
- **综合文档**自动创建技术报告
- **PDF导出**支持PDF输出
- **配置保存与使用**可以新建，保存，导出配置

## 🏗️ 系统架构

```

ICS-system-opt/
├──核心应用层
│├── main.cpp                    # 应用程序入口
│├── mainwindow.h/cpp           # 主界面控制器
│└── mainwindow.ui              # Qt Designer界面文件
├──CAN协议模块
│├── CANBitTiming.h/cpp         # 位时序计算引擎
│├── CanIdFilterLib.h/cpp       # ID分配与过滤器设计
│└── OptionalCanFeatures.hpp    # 高级CAN功能支持
├──网络设计模块
│├── NetworkDesigner.h/cpp      # 拓扑规划算法
│└── NetworkView.h/cpp          # 可视化组件
├──用户界面组件
│├── addbaud.h/cpp/ui           # 波特率配置对话框
│└── addnode.h/cpp/ui           # 节点配置对话框
└──报告系统
├── ComprehensiveReport.h/cpp  # 综合分析报告
└── PDFReportGenerator.h/cpp   # PDF导出引擎

```

## 📥 安装方法

### 环境需要

- **Qt框架**: Qt 5.12 或更高版本（支持 Qt6）
- **构建工具**: CMake 3.16+
- **编译器**: 支持C++17的编译器（GCC 7+、MinGW、Clang 6+、MSVC 2019+）

### 构建步骤

1. **克隆仓库**
   ```bash
   git clone https://github.com/starpicke/ICS-system-opt
   cd QoptimalICS

1. 配置项目
   ```bash
   mkdir build && cd build
   cmake ..
   ```
2. 编译构建
   ```bash
   cmake --build .
   # 或者使用 make（Linux/macOS）
   make -j4
   ```
3. 运行应用
   ```bash
   ./QoptimalICS  # Linux/macOS
   # 或直接运行生成的可执行文件（Windows）
   ```

 软件使用指南

详细的使用教程和操作说明请参阅：USER.md


🤝 贡献指南

欢迎社区贡献！请参阅CONTRIBUTING.md了解详情。

📄 许可证

本项目采用MIT许可证 - 详见LICENSE文件。
