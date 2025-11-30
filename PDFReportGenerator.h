/**
 * @file PDFReportGenerator.h
 * @brief PDF报告生成器 - 修正A4尺寸和文字显示
 */

#pragma once
#include "ComprehensiveReport.h"
#include <QString>
#include <QPrinter>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QPageSize>
#include <QPageLayout>

namespace pdfreport {

class PDFReportGenerator {
public:
    PDFReportGenerator();
    ~PDFReportGenerator() = default;

    bool GenerateTechnicalReport(const canproject::ComprehensiveReport& report,
                                 const QString& filename,
                                 const QString& screenshotPath = "");

private:
    // 🎯 修正的A4页面尺寸 (210mm × 297mm)
    const int PAGE_WIDTH_MM = 210;
    const int PAGE_HEIGHT_MM = 297;
    const int PAGE_WIDTH = 700;        // 210mm × 2.83465 ≈ 595点
    const int PAGE_HEIGHT = 842;       // 297mm × 2.83465 ≈ 842点
    const int PAGE_CENTER_X = PAGE_WIDTH / 2;  // 🎯 页面中心线X坐标

    // 🎯 内容区域参数
    const int CONTENT_WIDTH = 500;     // 主要内容区域宽度
    const int CONTENT_MARGIN_LEFT = (PAGE_WIDTH - CONTENT_WIDTH) / 2;

    // 🎯 字体大小参数
    const int COVER_TITLE_FONT_SIZE = 22;    // 封面标题字体减小
    const int COVER_SUBTITLE_FONT_SIZE = 14; // 封面副标题字体减小
    const int COVER_INFO_FONT_SIZE = 10;     // 封面信息字体减小
    const int TITLE_FONT_SIZE = 20;
    const int SECTION_FONT_SIZE = 14;
    const int CONTENT_FONT_SIZE = 10;

    // 🎯 间距参数
    const int LINE_HEIGHT = 18;
    const int PARAGRAPH_SPACING = 25;
    const int SECTION_SPACING = 40;

    // 绘制函数
    void DrawCoverPage(QPainter& painter, const canproject::ComprehensiveReport& report);
    void DrawDetailedContent(QPainter& painter, const canproject::ComprehensiveReport& report, const QString& screenshotPath);
    void DrawModuleDetails(QPainter& painter, const canproject::ComprehensiveReport& report, int startY, const QString& screenshotPath);

    // 🎯 基于中心线的绘制函数
    void DrawCenteredText(QPainter& painter, int y, int lineHeight, const QString& text, const QFont& font);
    void DrawCenteredLine(QPainter& painter, int y, int width);
    void DrawKeyValuePairCentered(QPainter& painter, int& y, const QString& key, const QString& value, int pairWidth = 400);

    // 表格绘制函数
    void DrawTableRowCentered(QPainter& painter, int& y, const QString& key, const QString& value, int tableWidth = 450);

    // 截图相关函数
    void DrawNetworkTopology(QPainter& painter, int& currentY, const QString& screenshotPath);
    bool DrawImage(QPainter& painter, const QString& imagePath, int x, int y, int maxWidth, int maxHeight);

    // 工具函数
    bool CheckNewPage(int y, int requiredHeight = 100);
    int CalculateTextHeight(QPainter& painter, const QString& text, int width);
    //网桥绘制
    void DrawBridgeRepeaterTable(QPainter& painter, int& currentY, const canproject::ComprehensiveReport& report);
    QString DetermineConnectedSegments(const std::pair<double, double>& devicePos, const canproject::ComprehensiveReport& report);



    QPrinter printer;
};

} // namespace pdfreport


