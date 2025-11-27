/**
 * @file PDFReportGenerator.cpp
 * @brief PDF报告生成器实现 - 完全居中版本
 */

#include "PDFReportGenerator.h"
#include <QDateTime>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

namespace pdfreport {

PDFReportGenerator::PDFReportGenerator() {
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setFullPage(true);
}

bool PDFReportGenerator::GenerateTechnicalReport(const canproject::ComprehensiveReport& report,
                                                 const QString& filename) {
    printer.setOutputFileName(filename);

    QPainter painter;
    if (!painter.begin(&printer)) {
        return false;
    }

    // 设置抗锯齿
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制封面 - 完全居中设计
    DrawCoverPage(painter, report);

    // 开始新页面
    printer.newPage();

    // 绘制详细内容
    DrawDetailedContent(painter, report);

    painter.end();
    return true;
}

void PDFReportGenerator::DrawCoverPage(QPainter& painter, const canproject::ComprehensiveReport& report) {
    // 清除背景
    painter.fillRect(QRect(0, 0, PAGE_WIDTH, PAGE_HEIGHT), Qt::white);

    // 计算可用区域
    int contentWidth = PAGE_WIDTH - 2 * PAGE_MARGIN;
    int centerX = PAGE_WIDTH / 2;

    // 设置字体
    QFont titleFont("Microsoft YaHei", 28, QFont::Bold);
    QFont subtitleFont("Microsoft YaHei", 16, QFont::Normal);
    QFont infoFont("Microsoft YaHei", 12, QFont::Normal);
    QFont statusFont("Microsoft YaHei", 14, QFont::Bold);
    QFont footerFont("Microsoft YaHei", 10, QFont::Normal);

    // 垂直居中计算
    int totalHeight = 500; // 预估总高度
    int startY = (PAGE_HEIGHT - totalHeight) / 2;
    int currentY = startY;

    // 1. 主标题 - 完全居中
    painter.setFont(titleFont);
    painter.setPen(QColor(0, 70, 130)); // 深蓝色

    QString mainTitle = "CAN总线网络参数优化系统";
    QFontMetrics titleMetrics(titleFont);
    int titleWidth = titleMetrics.horizontalAdvance(mainTitle);
    painter.drawText(centerX - titleWidth/2, currentY, mainTitle);
    currentY += 60;

    // 副标题
    painter.setFont(subtitleFont);
    painter.setPen(QColor(100, 100, 100));
    QString subTitle = "技术报告";
    QFontMetrics subTitleMetrics(subtitleFont);
    int subTitleWidth = subTitleMetrics.horizontalAdvance(subTitle);
    painter.drawText(centerX - subTitleWidth/2, currentY, subTitle);
    currentY += 80;

    // 2. 装饰线 - 居中
    painter.setPen(QPen(QColor(200, 200, 200), 2));
    int lineWidth = 300;
    painter.drawLine(centerX - lineWidth/2, currentY, centerX + lineWidth/2, currentY);
    currentY += 50;

    // 3. 项目信息表格 - 居中
    int infoBoxWidth = 400;
    int infoBoxX = centerX - infoBoxWidth/2;

    painter.setFont(infoFont);
    painter.setPen(Qt::black);

    // 项目名称
    DrawCenteredInfoRow(painter, infoBoxX, currentY, infoBoxWidth, "项目名称：", QString::fromStdString(report.projectName));
    currentY += 35;

    // 设计人员
    DrawCenteredInfoRow(painter, infoBoxX, currentY, infoBoxWidth, "设计人员：", QString::fromStdString(report.author));
    currentY += 35;

    // 生成时间
    DrawCenteredInfoRow(painter, infoBoxX, currentY, infoBoxWidth, "生成时间：", QString::fromStdString(report.timestamp));
    currentY += 35;

    // 项目描述
    DrawCenteredInfoRow(painter, infoBoxX, currentY, infoBoxWidth, "项目描述：", QString::fromStdString(report.description));
    currentY += 50;

    // 4. 状态信息 - 居中
    painter.setFont(statusFont);
    QString statusText = report.allCalculationsSuccessful ? "✅ 所有计算成功" : "⚠️ 存在计算警告";
    QColor statusColor = report.allCalculationsSuccessful ? QColor(0, 150, 0) : QColor(200, 120, 0);
    painter.setPen(statusColor);

    QFontMetrics statusMetrics(statusFont);
    int statusWidth = statusMetrics.horizontalAdvance(statusText);
    painter.drawText(centerX - statusWidth/2, currentY, statusText);
    currentY += 80;

    // 5. 底部信息 - 居中
    painter.setFont(footerFont);
    painter.setPen(QColor(100, 100, 100));

    QString footer1 = "CAN总线研究性专题项目";
    QFontMetrics footerMetrics(footerFont);
    int footer1Width = footerMetrics.horizontalAdvance(footer1);
    painter.drawText(centerX - footer1Width/2, PAGE_HEIGHT - 60, footer1);

    QString footer2 = QDateTime::currentDateTime().toString("yyyy年MM月dd日");
    int footer2Width = footerMetrics.horizontalAdvance(footer2);
    painter.drawText(centerX - footer2Width/2, PAGE_HEIGHT - 40, footer2);
}

void PDFReportGenerator::DrawDetailedContent(QPainter& painter, const canproject::ComprehensiveReport& report) {
    int centerX = PAGE_WIDTH / 2;
    int currentY = PAGE_MARGIN;

    // 页面标题
    DrawCenteredTitle(painter, currentY, "详细设计报告");
    currentY += 80;

    QFont contentFont("Microsoft YaHei", 11);
    QFont sectionFont("Microsoft YaHei", 13, QFont::Bold);
    QFont tableHeaderFont("Microsoft YaHei", 11, QFont::Bold);

    painter.setFont(contentFont);

    // 执行摘要
    DrawCenteredSectionTitle(painter, currentY, "执行摘要");
    currentY += 40;

    QString summaryText = "本报告基于CAN总线网络参数优化系统的设计结果生成，系统通过四个核心模块对网络参数进行全面优化，确保通信的可靠性、实时性和稳定性。";
    DrawCenteredMultiLineText(painter, currentY, 450, summaryText, contentFont);
    currentY += 80;

    // 主要优化成果表格
    DrawCenteredSectionTitle(painter, currentY, "主要优化成果");
    currentY += 40;

    int tableWidth = 500;
    int tableX = centerX - tableWidth/2;

    // 表头
    painter.setFont(tableHeaderFont);
    painter.setPen(QColor(70, 70, 150));

    painter.drawText(QRect(tableX, currentY, tableWidth/2, 25), Qt::AlignCenter, "优化指标");
    painter.drawText(QRect(tableX + tableWidth/2, currentY, tableWidth/2, 25), Qt::AlignCenter, "优化结果");
    currentY += 30;

    // 表格分隔线
    painter.setPen(QPen(QColor(200, 200, 200), 1));
    painter.drawLine(tableX, currentY, tableX + tableWidth, currentY);
    currentY += 10;

    // 表格内容
    painter.setFont(contentFont);
    painter.setPen(Qt::black);

    DrawCenteredTableRow(painter, currentY, tableX, tableWidth, "通信负载",
                         QString::number(report.baudRate.output.actualLoadPercent, 'f', 1) + "%");
    DrawCenteredTableRow(painter, currentY, tableX, tableWidth, "推荐波特率",
                         QString::number(report.baudRate.output.recommendedBaudRate) + " bps");
    DrawCenteredTableRow(painter, currentY, tableX, tableWidth, "网络拓扑",
                         QString::number(report.network.output.segments.size()) + "个网段");
    DrawCenteredTableRow(painter, currentY, tableX, tableWidth, "时序精度",
                         QString::number(report.bitTiming.output.errorPercent, 'f', 2) + "%误差");
    DrawCenteredTableRow(painter, currentY, tableX, tableWidth, "信号完整性",
                         QString::number(report.slopeControl.output.actualRiseTimeNs, 'f', 1) + "ns上升时间");

    currentY += 60;

    // 详细模块结果 - 检查是否需要新页面
    if (CheckNewPage(currentY, 400)) {
        printer.newPage();
        currentY = PAGE_MARGIN;
    }

    // 模块详情部分
    DrawModuleDetails(painter, report, currentY);
}

void PDFReportGenerator::DrawModuleDetails(QPainter& painter, const canproject::ComprehensiveReport& report, int startY) {
    int centerX = PAGE_WIDTH / 2;
    int currentY = startY;

    QFont moduleTitleFont("Microsoft YaHei", 14, QFont::Bold);
    QFont contentFont("Microsoft YaHei", 10);
    QFont tableHeaderFont("Microsoft YaHei", 10, QFont::Bold);

    int moduleWidth = 450;
    int moduleX = centerX - moduleWidth/2;

    // 1. 波特率计算模块
    painter.setFont(moduleTitleFont);
    painter.setPen(QColor(0, 100, 200));
    painter.drawText(QRect(moduleX, currentY, moduleWidth, 30), Qt::AlignCenter, "1. 波特率计算模块");
    currentY += 40;

    if (report.baudRate.output.calculationSuccess) {
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "计算状态", "✅ 成功");
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "总比特率",
                             QString::number(report.baudRate.output.totalBitRate, 'f', 0) + " bps");
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "推荐波特率",
                             QString::number(report.baudRate.output.recommendedBaudRate) + " bps");
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "实际负载率",
                             QString::number(report.baudRate.output.actualLoadPercent, 'f', 1) + "%");
    } else {
        painter.setPen(Qt::red);
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "计算状态", "❌ 失败");
        painter.setPen(Qt::black);
    }
    currentY += 60;

    // 检查分页
    if (CheckNewPage(currentY, 200)) {
        printer.newPage();
        currentY = PAGE_MARGIN;
    }

    // 2. 网络拓扑设计模块
    painter.setFont(moduleTitleFont);
    painter.setPen(QColor(0, 100, 200));
    painter.drawText(QRect(moduleX, currentY, moduleWidth, 30), Qt::AlignCenter, "2. 网络拓扑设计模块");
    currentY += 40;

    if (report.network.output.overallSuccess) {
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "设计状态", "✅ 成功");
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "网段数量",
                             QString::number(report.network.output.segments.size()) + "个");
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "终端电阻",
                             QString::number(report.network.output.devices.terminators.size()) + "个");
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "中继器数量",
                             QString::number(report.network.output.devices.repeaters.size()) + "个");
    } else {
        painter.setPen(Qt::red);
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "设计状态", "❌ 失败");
        painter.setPen(Qt::black);
    }
    currentY += 60;

    // 检查分页
    if (CheckNewPage(currentY, 200)) {
        printer.newPage();
        currentY = PAGE_MARGIN;
    }

    // 3. 位时序参数计算模块
    painter.setFont(moduleTitleFont);
    painter.setPen(QColor(0, 100, 200));
    painter.drawText(QRect(moduleX, currentY, moduleWidth, 30), Qt::AlignCenter, "3. 位时序参数计算模块");
    currentY += 40;

    if (report.bitTiming.output.calculationSuccess) {
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "计算状态", "✅ 成功");
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "BRP值",
                             QString::number(report.bitTiming.output.BRP));
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "TSEG1值",
                             QString::number(report.bitTiming.output.TSEG1));
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "TSEG2值",
                             QString::number(report.bitTiming.output.TSEG2));
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "实际波特率",
                             QString::number(report.bitTiming.output.actualBaudRate) + " bps");
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "计算误差",
                             QString::number(report.bitTiming.output.errorPercent, 'f', 2) + "%");
    } else {
        painter.setPen(Qt::red);
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "计算状态", "❌ 失败");
        painter.setPen(Qt::black);
    }
    currentY += 80;

    // 检查分页
    if (CheckNewPage(currentY, 150)) {
        printer.newPage();
        currentY = PAGE_MARGIN;
    }

    // 4. 斜率控制电阻选择模块
    painter.setFont(moduleTitleFont);
    painter.setPen(QColor(0, 100, 200));
    painter.drawText(QRect(moduleX, currentY, moduleWidth, 30), Qt::AlignCenter, "4. 斜率控制电阻选择模块");
    currentY += 40;

    if (report.slopeControl.output.calculationSuccess) {
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "计算状态", "✅ 成功");
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "推荐电阻",
                             QString::number(report.slopeControl.output.recommendedResistor, 'f', 1) + " Ω");
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "实际上升时间",
                             QString::number(report.slopeControl.output.actualRiseTimeNs, 'f', 1) + " ns");
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "工作模式",
                             QString::fromStdString(report.slopeControl.output.recommendedMode));
    } else {
        painter.setPen(Qt::red);
        DrawCenteredTableRow(painter, currentY, moduleX, moduleWidth, "计算状态", "❌ 失败");
        painter.setPen(Qt::black);
    }
}

// ==================== 辅助绘制函数 ====================

void PDFReportGenerator::DrawCenteredInfoRow(QPainter& painter, int x, int y, int width, const QString& key, const QString& value) {
    QFont boldFont("Microsoft YaHei", 12, QFont::Bold);
    QFont normalFont("Microsoft YaHei", 12, QFont::Normal);

    // 键（粗体，右对齐）
    painter.setFont(boldFont);
    painter.drawText(QRect(x, y, 100, 25), Qt::AlignRight, key);

    // 值（正常，左对齐）
    painter.setFont(normalFont);

    // 处理长文本
    QFontMetrics metrics(normalFont);
    QString displayValue = value;
    if (metrics.horizontalAdvance(displayValue) > width - 120) {
        displayValue = metrics.elidedText(displayValue, Qt::ElideRight, width - 120);
    }

    painter.drawText(QRect(x + 110, y, width - 110, 25), Qt::AlignLeft, displayValue);
}

void PDFReportGenerator::DrawCenteredTitle(QPainter& painter, int& yPos, const QString& title) {
    QFont titleFont("Microsoft YaHei", 22, QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(QColor(0, 70, 130));

    QFontMetrics metrics(titleFont);
    int titleWidth = metrics.horizontalAdvance(title);
    int centerX = PAGE_WIDTH / 2;

    painter.drawText(centerX - titleWidth/2, yPos, title);
    yPos += 40;

    // 装饰线
    painter.setPen(QPen(QColor(200, 200, 230), 3));
    int lineWidth = 200;
    painter.drawLine(centerX - lineWidth/2, yPos, centerX + lineWidth/2, yPos);
    yPos += 30;
}

void PDFReportGenerator::DrawCenteredSectionTitle(QPainter& painter, int& yPos, const QString& title) {
    QFont sectionFont("Microsoft YaHei", 16, QFont::Bold);
    painter.setFont(sectionFont);
    painter.setPen(QColor(0, 90, 160));

    QFontMetrics metrics(sectionFont);
    int titleWidth = metrics.horizontalAdvance(title);
    int centerX = PAGE_WIDTH / 2;

    painter.drawText(centerX - titleWidth/2, yPos, title);
    yPos += 30;
}

void PDFReportGenerator::DrawCenteredMultiLineText(QPainter& painter, int& yPos, int maxWidth, const QString& text, const QFont& font) {
    painter.setFont(font);

    QFontMetrics metrics(font);
    int centerX = PAGE_WIDTH / 2;
    int textX = centerX - maxWidth/2;

    // 简单的文本换行
    QString currentLine;
    QStringList words = text.split(' ');

    for (const QString& word : words) {
        QString testLine = currentLine.isEmpty() ? word : currentLine + " " + word;
        if (metrics.horizontalAdvance(testLine) <= maxWidth) {
            currentLine = testLine;
        } else {
            if (!currentLine.isEmpty()) {
                painter.drawText(textX, yPos, currentLine);
                yPos += 25;
            }
            currentLine = word;
        }
    }

    if (!currentLine.isEmpty()) {
        painter.drawText(textX, yPos, currentLine);
        yPos += 25;
    }
}

void PDFReportGenerator::DrawCenteredTableRow(QPainter& painter, int& yPos, int x, int width, const QString& key, const QString& value) {
    QFont normalFont("Microsoft YaHei", 10);
    painter.setFont(normalFont);

    // 键（左对齐）
    painter.drawText(QRect(x, yPos, width/2 - 10, 20), Qt::AlignRight, key);

    // 值（左对齐）
    painter.drawText(QRect(x + width/2 + 10, yPos, width/2 - 10, 20), Qt::AlignLeft, value);

    yPos += 22;
}

bool PDFReportGenerator::CheckNewPage(int yPos, int requiredHeight) {
    return (yPos + requiredHeight) > (PAGE_HEIGHT - PAGE_MARGIN);
}

bool PDFReportGenerator::GenerateNetworkSummary(const canproject::ComprehensiveReport& report,
                                                const QString& filename) {
    // 简化的网络摘要PDF生成
    printer.setOutputFileName(filename);

    QPainter painter;
    if (!painter.begin(&printer)) {
        return false;
    }

    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制封面
    DrawCoverPage(painter, report);

    painter.end();
    return true;
}

} // namespace pdfreport
