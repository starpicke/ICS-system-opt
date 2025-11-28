/**
 * @file PDFReportGenerator.cpp
 * @brief PDF报告生成器实现 - 完整版本
 */

#include "PDFReportGenerator.h"
#include <QDateTime>
#include <QDebug>
#include <QFile>

namespace pdfreport {

PDFReportGenerator::PDFReportGenerator() {
    // 🎯 设置正确的A4页面尺寸
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::A4)); // 使用Qt内置的A4定义
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setFullPage(false);
    printer.setPageMargins(QMarginsF(10, 10, 10, 10)); // 设置适当边距
}

bool PDFReportGenerator::GenerateTechnicalReport(const canproject::ComprehensiveReport& report,
                                                 const QString& filename,
                                                 const QString& screenshotPath) {
    printer.setOutputFileName(filename);

    QPainter painter;
    if (!painter.begin(&printer)) {
        return false;
    }

    // 设置抗锯齿
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // 绘制封面
    DrawCoverPage(painter, report);

    // 开始新页面
    printer.newPage();

    // 绘制详细内容，传递截图路径
    DrawDetailedContent(painter, report, screenshotPath);

    painter.end();
    return true;
}

void PDFReportGenerator::DrawCoverPage(QPainter& painter, const canproject::ComprehensiveReport& report) {
    // 清除背景
    painter.fillRect(QRect(0, 0, PAGE_WIDTH, PAGE_HEIGHT), Qt::white);

    // 🎯 页面垂直中心
    int centerY = PAGE_HEIGHT / 2;

    // 设置字体 - 使用修正后的字体大小
    QFont titleFont("Microsoft YaHei", COVER_TITLE_FONT_SIZE, QFont::Bold);
    QFont subtitleFont("Microsoft YaHei", COVER_SUBTITLE_FONT_SIZE, QFont::Normal);
    QFont infoFont("Microsoft YaHei", COVER_INFO_FONT_SIZE, QFont::Normal);
    QFont statusFont("Microsoft YaHei", COVER_INFO_FONT_SIZE + 2, QFont::Bold);
    QFont footerFont("Microsoft YaHei", 9, QFont::Normal);

    // ========== 🎯 基于页面中心线居中绘制所有内容 ==========

    // 🎯 1. 主标题 - 完全居中，增加行高确保显示完整
    int titleStartY = centerY - 180;
    DrawCenteredText(painter, titleStartY, 60, "CAN总线网络参数优化系统技术报告", titleFont);

    // 🎯 2. 副标题
    DrawCenteredText(painter, titleStartY + 60, 30, "技术报告", subtitleFont);

    // 🎯 3. 装饰线 - 居中
    DrawCenteredLine(painter, titleStartY + 100, 280);

    // 🎯 4. 直接显示项目信息内容（不显示标签）
    int infoStartY = titleStartY + 140;

    painter.setFont(infoFont);
    painter.setPen(Qt::black);

    int contentWidth = 420; // 内容区域宽度
    int contentX = PAGE_CENTER_X - contentWidth / 2;

    // 项目名称 - 直接显示内容
    if (!report.projectName.empty()) {
        QRect nameRect(contentX, infoStartY, contentWidth, 25);
        painter.drawText(nameRect, Qt::AlignCenter, QString::fromStdString(report.projectName));
        infoStartY += 30;
    }

    // 设计人员 - 直接显示内容
    if (!report.author.empty()) {
        QRect authorRect(contentX, infoStartY, contentWidth, 25);
        painter.drawText(authorRect, Qt::AlignCenter, QString::fromStdString(report.author));
        infoStartY += 30;
    }

    // 生成时间 - 直接显示内容
    if (!report.timestamp.empty()) {
        QRect timeRect(contentX, infoStartY, contentWidth, 25);
        painter.drawText(timeRect, Qt::AlignCenter, QString::fromStdString(report.timestamp));
        infoStartY += 30;
    }

    // 项目描述 - 直接显示内容（可能需要多行）
    if (!report.description.empty()) {
        QString description = QString::fromStdString(report.description);
        QFontMetrics descMetrics(infoFont);

        // 计算描述文本的高度
        int descHeight = CalculateTextHeight(painter, description, contentWidth);
        QRect descRect(contentX, infoStartY, contentWidth, descHeight);
        painter.drawText(descRect, Qt::AlignCenter | Qt::TextWordWrap, description);
        infoStartY += descHeight + 20;
    }

    // 🎯 5. 状态信息 - 居中
    QString statusText = report.allCalculationsSuccessful ? "✅ 所有计算成功" : "⚠️ 存在计算警告";
    QColor statusColor = report.allCalculationsSuccessful ? QColor(0, 150, 0) : QColor(200, 120, 0);

    painter.setFont(statusFont);
    painter.setPen(statusColor);
    DrawCenteredText(painter, infoStartY, 30, statusText, statusFont);

    // 🎯 6. 底部信息 - 居中
    painter.setPen(QColor(100, 100, 100));
    painter.setFont(footerFont);

    DrawCenteredText(painter, PAGE_HEIGHT - 60, 20, "CAN总线研究性专题项目", footerFont);
    DrawCenteredText(painter, PAGE_HEIGHT - 35, 20, QDateTime::currentDateTime().toString("yyyy年MM月dd日"), footerFont);
}

void PDFReportGenerator::DrawDetailedContent(QPainter& painter, const canproject::ComprehensiveReport& report, const QString& screenshotPath) {
    int currentY = 80; // 页面顶部边距

    // 🎯 页面标题 - 完全居中
    DrawCenteredText(painter, currentY, 40, "详细设计报告", QFont("Microsoft YaHei", TITLE_FONT_SIZE, QFont::Bold));
    currentY += 60;

    // 🎯 执行摘要标题
    painter.setFont(QFont("Microsoft YaHei", SECTION_FONT_SIZE, QFont::Bold));
    painter.setPen(QColor(0, 90, 160));

    QRect summaryTitleRect(CONTENT_MARGIN_LEFT, currentY, CONTENT_WIDTH, 25);
    painter.drawText(summaryTitleRect, Qt::AlignLeft, "执行摘要");
    currentY += 35;

    // 🎯 执行摘要内容 - 计算实际高度
    QString summaryText = "本报告基于CAN总线网络参数优化系统的设计结果生成，系统通过四个核心模块对网络参数进行全面优化，确保通信的可靠性、实时性和稳定性。";

    painter.setFont(QFont("Microsoft YaHei", CONTENT_FONT_SIZE));
    painter.setPen(Qt::black);

    int textHeight = CalculateTextHeight(painter, summaryText, CONTENT_WIDTH);
    QRect summaryContentRect(CONTENT_MARGIN_LEFT, currentY, CONTENT_WIDTH, textHeight);
    painter.drawText(summaryContentRect, Qt::AlignLeft | Qt::TextWordWrap, summaryText);
    currentY += textHeight + PARAGRAPH_SPACING;

    // 🎯 主要优化成果标题
    painter.setFont(QFont("Microsoft YaHei", SECTION_FONT_SIZE, QFont::Bold));
    painter.setPen(QColor(0, 90, 160));

    QRect resultsTitleRect(CONTENT_MARGIN_LEFT, currentY, CONTENT_WIDTH, 25);
    painter.drawText(resultsTitleRect, Qt::AlignLeft, "主要优化成果");
    currentY += 35;

    // 🎯 优化成果表格 - 居中
    painter.setFont(QFont("Microsoft YaHei", CONTENT_FONT_SIZE, QFont::Bold));
    painter.setPen(QColor(70, 70, 150));

    // 表头
    DrawTableRowCentered(painter, currentY, "优化指标", "优化结果", CONTENT_WIDTH);
    currentY += 5; // 表头与内容间距

    // 表格内容
    painter.setFont(QFont("Microsoft YaHei", CONTENT_FONT_SIZE));
    painter.setPen(Qt::black);

    DrawTableRowCentered(painter, currentY, "通信负载", QString::number(report.baudRate.output.actualLoadPercent, 'f', 1) + "%", CONTENT_WIDTH);
    DrawTableRowCentered(painter, currentY, "推荐波特率", QString::number(report.baudRate.output.recommendedBaudRate) + " bps", CONTENT_WIDTH);
    DrawTableRowCentered(painter, currentY, "网络拓扑", QString::number(report.network.output.segments.size()) + "个网段", CONTENT_WIDTH);
    DrawTableRowCentered(painter, currentY, "时序精度", QString::number(report.bitTiming.output.errorPercent, 'f', 2) + "%误差", CONTENT_WIDTH);
    DrawTableRowCentered(painter, currentY, "信号完整性", QString::number(report.slopeControl.output.actualRiseTimeNs, 'f', 1) + "ns上升时间", CONTENT_WIDTH);

    currentY += 40;

    // 详细模块结果
    if (CheckNewPage(currentY, 600)) {
        printer.newPage();
        currentY = 80;
    }

    DrawModuleDetails(painter, report, currentY, screenshotPath);
}

void PDFReportGenerator::DrawModuleDetails(QPainter& painter, const canproject::ComprehensiveReport& report, int startY, const QString& screenshotPath) {
    int currentY = startY;

    QFont moduleTitleFont("Microsoft YaHei", SECTION_FONT_SIZE, QFont::Bold);
    QFont contentFont("Microsoft YaHei", CONTENT_FONT_SIZE);
    QFont smallFont("Microsoft YaHei", CONTENT_FONT_SIZE - 1);

    // 1. 波特率计算模块
    painter.setFont(moduleTitleFont);
    painter.setPen(QColor(0, 100, 200));

    QRect module1TitleRect(CONTENT_MARGIN_LEFT, currentY, CONTENT_WIDTH, 25);
    painter.drawText(module1TitleRect, Qt::AlignLeft, "1. 波特率计算模块");
    currentY += 40;

    if (report.baudRate.output.calculationSuccess) {
        painter.setFont(contentFont);
        painter.setPen(Qt::black);
        DrawTableRowCentered(painter, currentY, "计算状态", "✅ 成功", CONTENT_WIDTH);
        DrawTableRowCentered(painter, currentY, "总比特率", QString::number(report.baudRate.output.totalBitRate, 'f', 0) + " bps", CONTENT_WIDTH);
        DrawTableRowCentered(painter, currentY, "推荐波特率", QString::number(report.baudRate.output.recommendedBaudRate) + " bps", CONTENT_WIDTH);
        DrawTableRowCentered(painter, currentY, "实际负载率", QString::number(report.baudRate.output.actualLoadPercent, 'f', 1) + "%", CONTENT_WIDTH);

        // 输入参数
        currentY += 20;
        painter.setFont(smallFont);
        painter.setPen(QColor(100, 100, 100));
        DrawTableRowCentered(painter, currentY, "期望负载率", QString::number(report.baudRate.input.desiredLoadPercent) + "%", CONTENT_WIDTH);
        DrawTableRowCentered(painter, currentY, "节点数据数量", QString::number(report.baudRate.input.nodeDataList.size()) + "个", CONTENT_WIDTH);
    }
    currentY += 50;

    // 检查分页
    if (CheckNewPage(currentY, 400)) {
        printer.newPage();
        currentY = 80;
    }

    // 2. 网络拓扑设计模块
    painter.setFont(moduleTitleFont);
    painter.setPen(QColor(0, 100, 200));

    QRect module2TitleRect(CONTENT_MARGIN_LEFT, currentY, CONTENT_WIDTH, 25);
    painter.drawText(module2TitleRect, Qt::AlignLeft, "2. 网络拓扑设计模块");
    currentY += 40;

    if (report.network.output.overallSuccess) {
        painter.setFont(contentFont);
        painter.setPen(Qt::black);
        DrawTableRowCentered(painter, currentY, "设计状态", "✅ 成功", CONTENT_WIDTH);
        DrawTableRowCentered(painter, currentY, "网段数量", QString::number(report.network.output.segments.size()) + "个", CONTENT_WIDTH);
        DrawTableRowCentered(painter, currentY, "终端电阻", QString::number(report.network.output.devices.terminators.size()) + "个", CONTENT_WIDTH);
        DrawTableRowCentered(painter, currentY, "中继器数量", QString::number(report.network.output.devices.repeaters.size()) + "个", CONTENT_WIDTH);
        DrawTableRowCentered(painter, currentY, "网桥数量", QString::number(report.network.output.devices.bridges.size()) + "个", CONTENT_WIDTH);

        // 终端电阻位置表格
        if (!report.network.output.devices.terminators.empty()) {
            currentY += 30;
            painter.setFont(moduleTitleFont);
            painter.setPen(QColor(0, 100, 200));
            QRect terminatorTitleRect(CONTENT_MARGIN_LEFT, currentY, CONTENT_WIDTH, 25);
            painter.drawText(terminatorTitleRect, Qt::AlignLeft, "终端电阻位置");
            currentY += 30;

            painter.setFont(smallFont);
            painter.setPen(QColor(70, 70, 150));
            DrawTableRowCentered(painter, currentY, "序号", "位置坐标(X,Y)", CONTENT_WIDTH);
            currentY += 5;

            painter.setPen(Qt::black);
            for (size_t i = 0; i < report.network.output.devices.terminators.size(); ++i) {
                const auto& terminator = report.network.output.devices.terminators[i];
                QString position = QString("(%1, %2)").arg(terminator.first, 0, 'f', 2).arg(terminator.second, 0, 'f', 2);
                DrawTableRowCentered(painter, currentY, QString::number(i + 1), position, CONTENT_WIDTH);
            }
        }

        // 网络拓扑图 - 使用截图
        currentY += 30;
        DrawNetworkTopology(painter, currentY, screenshotPath);
    }
    currentY += 50;

    // 检查分页
    if (CheckNewPage(currentY, 400)) {
        printer.newPage();
        currentY = 80;
    }

    // 3. 位时序配置模块
    painter.setFont(moduleTitleFont);
    painter.setPen(QColor(0, 100, 200));

    QRect module3TitleRect(CONTENT_MARGIN_LEFT, currentY, CONTENT_WIDTH, 25);
    painter.drawText(module3TitleRect, Qt::AlignLeft, "3. 位时序配置模块");
    currentY += 40;

    if (report.bitTiming.output.calculationSuccess) {
        painter.setFont(contentFont);
        painter.setPen(Qt::black);
        DrawTableRowCentered(painter, currentY, "计算状态", "✅ 成功", CONTENT_WIDTH);
        DrawTableRowCentered(painter, currentY, "误差百分比", QString::number(report.bitTiming.output.errorPercent, 'f', 2) + "%", CONTENT_WIDTH);
        DrawTableRowCentered(painter, currentY, "实际波特率", QString::number(report.bitTiming.output.actualBaudRate) + " bps", CONTENT_WIDTH);

        // 寄存器参数
        currentY += 20;
        painter.setFont(moduleTitleFont);
        painter.setPen(QColor(0, 100, 200));
        QRect registerTitleRect(CONTENT_MARGIN_LEFT, currentY, CONTENT_WIDTH, 25);
        painter.drawText(registerTitleRect, Qt::AlignLeft, "寄存器参数配置");
        currentY += 30;

        painter.setFont(contentFont);
        painter.setPen(Qt::black);
        DrawTableRowCentered(painter, currentY, "BRP (波特率预分频)", QString::number(report.bitTiming.output.BRP), CONTENT_WIDTH);
        DrawTableRowCentered(painter, currentY, "SJW (同步跳转宽度)", QString::number(report.bitTiming.output.SJW), CONTENT_WIDTH);
        DrawTableRowCentered(painter, currentY, "TSEG1 (时间段1)", QString::number(report.bitTiming.output.TSEG1), CONTENT_WIDTH);
        DrawTableRowCentered(painter, currentY, "TSEG2 (时间段2)", QString::number(report.bitTiming.output.TSEG2), CONTENT_WIDTH);
        DrawTableRowCentered(painter, currentY, "BTR寄存器", "0x" + QString::number(report.bitTiming.output.btrRegister, 16).toUpper(), CONTENT_WIDTH);

        // 输入参数
        currentY += 30;
        painter.setFont(smallFont);
        painter.setPen(QColor(100, 100, 100));
        DrawTableRowCentered(painter, currentY, "系统时钟", QString::number(report.bitTiming.input.systemClock) + " Hz", CONTENT_WIDTH);
        DrawTableRowCentered(painter, currentY, "目标波特率", QString::number(report.bitTiming.input.targetBaudRate) + " bps", CONTENT_WIDTH);
        DrawTableRowCentered(painter, currentY, "最大允许误差", QString::number(report.bitTiming.input.maxErrorPercent, 'f', 1) + "%", CONTENT_WIDTH);
    }
    currentY += 50;

    // 检查分页
    if (CheckNewPage(currentY, 200)) { // 减少所需高度，因为斜率控制模块内容减少了
        printer.newPage();
        currentY = 80;
    }

    // 4. 斜率控制模块 - 简化版本
    painter.setFont(moduleTitleFont);
    painter.setPen(QColor(0, 100, 200));

    QRect module4TitleRect(CONTENT_MARGIN_LEFT, currentY, CONTENT_WIDTH, 25);
    painter.drawText(module4TitleRect, Qt::AlignLeft, "4. 斜率控制模块");
    currentY += 40;

    if (report.slopeControl.output.calculationSuccess) {
        painter.setFont(contentFont);
        painter.setPen(Qt::black);
        DrawTableRowCentered(painter, currentY, "计算状态", "✅ 成功", CONTENT_WIDTH);
        DrawTableRowCentered(painter, currentY, "推荐电阻值", QString::number(report.slopeControl.output.recommendedResistor, 'f', 1) + " Ω", CONTENT_WIDTH);
        DrawTableRowCentered(painter, currentY, "实际上升时间", QString::number(report.slopeControl.output.actualRiseTimeNs, 'f', 1) + " ns", CONTENT_WIDTH);

        // 移除推荐工作模式和输入参数部分
    }
}


// ==================== 🎯 辅助函数实现 ====================

void PDFReportGenerator::DrawCenteredText(QPainter& painter, int y, int lineHeight, const QString& text, const QFont& font) {
    painter.setFont(font);
    QRect textRect(0, y, PAGE_WIDTH, lineHeight);
    painter.drawText(textRect, Qt::AlignCenter, text);
}

void PDFReportGenerator::DrawCenteredLine(QPainter& painter, int y, int width) {
    painter.setPen(QPen(QColor(150, 150, 150), 1));
    painter.drawLine(PAGE_CENTER_X - width/2, y, PAGE_CENTER_X + width/2, y);
}

void PDFReportGenerator::DrawKeyValuePairCentered(QPainter& painter, int& y, const QString& key, const QString& value, int pairWidth) {
    int pairX = PAGE_CENTER_X - pairWidth/2;

    painter.setFont(QFont("Microsoft YaHei", COVER_INFO_FONT_SIZE, QFont::Normal));
    painter.setPen(Qt::black);

    QRect keyRect(pairX, y, pairWidth/2, 25);
    QRect valueRect(pairX + pairWidth/2, y, pairWidth/2, 25);

    painter.drawText(keyRect, Qt::AlignRight | Qt::AlignVCenter, key);
    painter.drawText(valueRect, Qt::AlignLeft | Qt::AlignVCenter, value);

    y += 30;
}

void PDFReportGenerator::DrawTableRowCentered(QPainter& painter, int& y, const QString& key, const QString& value, int tableWidth) {
    int tableX = PAGE_CENTER_X - tableWidth/2;

    QRect keyRect(tableX, y, tableWidth/2 - 15, LINE_HEIGHT);
    QRect valueRect(tableX + tableWidth/2 + 15, y, tableWidth/2 - 15, LINE_HEIGHT);

    painter.drawText(keyRect, Qt::AlignRight | Qt::AlignVCenter, key);
    painter.drawText(valueRect, Qt::AlignLeft | Qt::AlignVCenter, value);

    y += LINE_HEIGHT + 3;
}

bool PDFReportGenerator::CheckNewPage(int y, int requiredHeight) {
    return (y + requiredHeight) > (PAGE_HEIGHT - 80); // 底部保留80点的边距
}

int PDFReportGenerator::CalculateTextHeight(QPainter& painter, const QString& text, int width) {
    QFontMetrics metrics(painter.font());
    QRect boundingRect = metrics.boundingRect(QRect(0, 0, width, 1000), Qt::TextWordWrap, text);
    return boundingRect.height() + 10; // 增加一些padding
}

// ==================== 截图相关函数实现 ====================

void PDFReportGenerator::DrawNetworkTopology(QPainter& painter, int& currentY, const QString& screenshotPath) {
    painter.setFont(QFont("Microsoft YaHei", SECTION_FONT_SIZE, QFont::Bold));
    painter.setPen(QColor(0, 100, 200));

    QRect topologyTitleRect(CONTENT_MARGIN_LEFT, currentY, CONTENT_WIDTH, 25);
    painter.drawText(topologyTitleRect, Qt::AlignLeft, "网络拓扑结构图");
    currentY += 30;

    if (!screenshotPath.isEmpty() && QFile::exists(screenshotPath)) {
        // 尝试绘制截图
        if (DrawImage(painter, screenshotPath, CONTENT_MARGIN_LEFT, currentY, CONTENT_WIDTH, 250)) {
            currentY += 260; // 图片高度 + 间距

            // 添加图片说明
            painter.setFont(QFont("Microsoft YaHei", CONTENT_FONT_SIZE - 1));
            painter.setPen(QColor(100, 100, 100));
            QRect captionRect(CONTENT_MARGIN_LEFT, currentY, CONTENT_WIDTH, 20);
            painter.drawText(captionRect, Qt::AlignCenter, "网络拓扑可视化图");
            currentY += 30;
            return;
        }
    }

    // 如果没有截图或截图加载失败，显示提示信息
    painter.setFont(QFont("Microsoft YaHei", CONTENT_FONT_SIZE));
    painter.setPen(Qt::black);

    QString noImageMsg = "网络拓扑可视化图生成成功。\n";
    noImageMsg += "详细的可视化网络结构请在软件界面中查看NetworkView窗口。";

    int msgHeight = CalculateTextHeight(painter, noImageMsg, CONTENT_WIDTH);
    QRect msgRect(CONTENT_MARGIN_LEFT, currentY, CONTENT_WIDTH, msgHeight);
    painter.drawText(msgRect, Qt::AlignCenter | Qt::TextWordWrap, noImageMsg);
    currentY += msgHeight + 20;
}

bool PDFReportGenerator::DrawImage(QPainter& painter, const QString& imagePath, int x, int y, int maxWidth, int maxHeight) {
    QPixmap image(imagePath);
    if (image.isNull()) {
        qDebug() << "无法加载图片:" << imagePath;
        return false;
    }

    // 调整图片大小以适应指定区域
    QPixmap scaledImage = image.scaled(maxWidth, maxHeight,
                                       Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 居中绘制图片
    int centeredX = x + (maxWidth - scaledImage.width()) / 2;
    painter.drawPixmap(centeredX, y, scaledImage);

    return true;
}

} // namespace pdfreport

