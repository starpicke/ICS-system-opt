/**
 * @file PDFReportGenerator.h
 * @brief PDF报告生成器 - 使用Qt生成真正PDF
 */

#pragma once
#include "ComprehensiveReport.h"
#include <QString>
#include <QPrinter>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>

    namespace pdfreport {

    class PDFReportGenerator {
    public:
        PDFReportGenerator();
        ~PDFReportGenerator() = default;

        /**
     * @brief 生成综合技术报告PDF
     */
        bool GenerateTechnicalReport(const canproject::ComprehensiveReport& report,
                                     const QString& filename);

        /**
     * @brief 生成网络拓扑摘要PDF
     */
        bool GenerateNetworkSummary(const canproject::ComprehensiveReport& report,
                                    const QString& filename);

    private:
        /**
     * @brief 绘制报告封面
     */
        void DrawCoverPage(QPainter& painter, const canproject::ComprehensiveReport& report);

        /**
     * @brief 绘制详细内容
     */
        void DrawDetailedContent(QPainter& painter, const canproject::ComprehensiveReport& report);

        /**
     * @brief 绘制模块详情
     */
        void DrawModuleDetails(QPainter& painter, const canproject::ComprehensiveReport& report, int startY);

        /**
     * @brief 绘制居中信息行
     */
        void DrawCenteredInfoRow(QPainter& painter, int x, int y, int width, const QString& key, const QString& value);

        /**
     * @brief 绘制居中标题
     */
        void DrawCenteredTitle(QPainter& painter, int& yPos, const QString& title);

        /**
     * @brief 绘制居中章节标题
     */
        void DrawCenteredSectionTitle(QPainter& painter, int& yPos, const QString& title);

        /**
     * @brief 绘制居中多行文本
     */
        void DrawCenteredMultiLineText(QPainter& painter, int& yPos, int maxWidth, const QString& text, const QFont& font);

        /**
     * @brief 绘制居中表格行
     */
        void DrawCenteredTableRow(QPainter& painter, int& yPos, int x, int width, const QString& key, const QString& value);

        /**
     * @brief 检查是否需要新页面
     */
        bool CheckNewPage(int yPos, int requiredHeight);

        QPrinter printer;
        const int PAGE_MARGIN = 50;
        const int PAGE_WIDTH = 595;  // A4 width in points
        const int PAGE_HEIGHT = 842; // A4 height in points
    };

} // namespace pdfreport
