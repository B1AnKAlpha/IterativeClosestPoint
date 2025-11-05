#include "ui/pages/reportpage.h"

#include <ElaLineEdit.h>
#include <ElaPushButton.h>
#include <ElaText.h>

#include <QDateEdit>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QPalette>
#include <QRegularExpression>
#include <QSpinBox>
#include <QStringList>
#include <QVBoxLayout>
#include <QStandardPaths>

namespace {
QStringList reportTemplates()
{
    return {
        QObject::tr("每日运行概览"),
        QObject::tr("车辆行程对账"),
        QObject::tr("异常事件周报"),
        QObject::tr("网约车效率分析"),
        QObject::tr("能耗与里程统计"),
    };
}
}

ReportPage::ReportPage(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("报表中心"));
    buildLayout();
    connectSignals();
}

void ReportPage::setReportService(ReportService *service)
{
    if (m_service == service) {
        return;
    }

    if (m_service) {
        disconnect(m_service, nullptr, this, nullptr);
    }

    m_service = service;
    if (!m_service) {
        return;
    }

    applyConfig(m_service->config());

    connect(m_service, &ReportService::reportGenerated, this, [this](const QString &path) {
        m_hintLabel->setText(tr("PDF 已导出至 %1").arg(path));
        QMessageBox::information(this, tr("导出成功"), tr("报表已生成：%1").arg(path));
    });
    connect(m_service, &ReportService::reportGenerationFailed, this, [this](const QString &reason) {
        m_hintLabel->setText(reason);
        QMessageBox::warning(this, tr("生成失败"), reason);
    });
    connect(m_service, &ReportService::configChanged, this, [this](const ReportService::ReportConfig &config) {
        applyConfig(config);
    });
}

void ReportPage::buildLayout()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(20);

    auto *title = new ElaText(tr("报表中心"), this);
    title->setTextStyle(ElaTextType::Title);
    title->setTextPixelSize(22);
    layout->addWidget(title);

    auto *subtitle = new ElaText(tr("一键生成运营报表，支持模板定制与定时推送"), this);
    subtitle->setTextStyle(ElaTextType::Caption);
    subtitle->setTextPixelSize(13);
    auto captionPalette = subtitle->palette();
    captionPalette.setColor(QPalette::WindowText, captionPalette.color(QPalette::WindowText).lighter(140));
    subtitle->setPalette(captionPalette);
    layout->addWidget(subtitle);

    auto *grid = new QGridLayout();
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(12);

    auto *scheduleTitle = new ElaText(tr("推送计划"), this);
    scheduleTitle->setTextStyle(ElaTextType::Subtitle);
    scheduleTitle->setTextPixelSize(16);
    grid->addWidget(scheduleTitle, 0, 0);

    m_scheduleEdit = new ElaLineEdit(this);
    m_scheduleEdit->setPlaceholderText(tr("示例：工作日 08:30 自动推送"));
    grid->addWidget(m_scheduleEdit, 0, 1);

    auto *ownerTitle = new ElaText(tr("责任人"), this);
    ownerTitle->setTextStyle(ElaTextType::Subtitle);
    ownerTitle->setTextPixelSize(16);
    grid->addWidget(ownerTitle, 1, 0);

    m_ownerEdit = new ElaLineEdit(this);
    m_ownerEdit->setPlaceholderText(tr("示例：刘昕, 王源"));
    grid->addWidget(m_ownerEdit, 1, 1);

    auto *autoArchiveTitle = new ElaText(tr("自动归档"), this);
    autoArchiveTitle->setTextStyle(ElaTextType::Subtitle);
    autoArchiveTitle->setTextPixelSize(16);
    grid->addWidget(autoArchiveTitle, 2, 0);

    m_archiveSpin = new QSpinBox(this);
    m_archiveSpin->setRange(7, 365);
    m_archiveSpin->setSuffix(tr(" 天"));
    grid->addWidget(m_archiveSpin, 2, 1);

    layout->addLayout(grid);

    m_hintLabel = new ElaText(tr("选择左侧模板以查看详情"), this);
    m_hintLabel->setTextStyle(ElaTextType::Caption);
    m_hintLabel->setTextPixelSize(13);
    layout->addWidget(m_hintLabel);

    auto *contentRow = new QHBoxLayout();
    contentRow->setSpacing(20);

    m_templateList = new QListWidget(this);
    m_templateList->setFrameShape(QFrame::NoFrame);
    m_templateList->addItems(reportTemplates());
    m_templateList->setSpacing(6);
    m_templateList->setCurrentRow(0);
    contentRow->addWidget(m_templateList, 1);

    auto *actionColumn = new QVBoxLayout();
    actionColumn->setSpacing(12);

    auto *previewTitle = new ElaText(tr("日报生成"), this);
    previewTitle->setTextStyle(ElaTextType::Subtitle);
    previewTitle->setTextPixelSize(16);
    actionColumn->addWidget(previewTitle);

    auto *previewInfo = new ElaText(tr("选择日期并导出 PDF，支持自动归档配置"), this);
    previewInfo->setTextStyle(ElaTextType::Body);
    previewInfo->setWordWrap(true);
    actionColumn->addWidget(previewInfo);

    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setCalendarPopup(true);
    actionColumn->addWidget(m_dateEdit);

    m_generateButton = new ElaPushButton(tr("导出日报 PDF"), this);
    m_generateButton->setMinimumHeight(38);
    actionColumn->addWidget(m_generateButton);

    m_scheduleSaveButton = new ElaPushButton(tr("保存推送配置"), this);
    m_scheduleSaveButton->setMinimumHeight(38);
    actionColumn->addWidget(m_scheduleSaveButton);

    actionColumn->addStretch(1);
    contentRow->addLayout(actionColumn, 1);

    layout->addLayout(contentRow);
    layout->addStretch(1);
}

void ReportPage::connectSignals()
{
    connect(m_templateList, &QListWidget::currentTextChanged, this, [this](const QString &text) {
        if (text.isEmpty()) {
            m_hintLabel->setText(tr("选择左侧模板以查看详情"));
            return;
        }
        m_hintLabel->setText(tr("已选模板：%1").arg(text));
    });

    connect(m_scheduleSaveButton, &ElaPushButton::clicked, this, [this]() {
        if (!m_service) {
            QMessageBox::warning(this, tr("配置未保存"), tr("报表服务尚未初始化"));
            return;
        }
        m_service->updateSchedule(m_scheduleEdit->text());
        m_service->updateOwners(parseOwners(m_ownerEdit->text()));
        m_service->updateArchiveDays(m_archiveSpin->value());
        m_hintLabel->setText(tr("推送配置已保存"));
    });

    connect(m_generateButton, &ElaPushButton::clicked, this, [this]() {
        if (!m_service) {
            QMessageBox::warning(this, tr("无法生成"), tr("报表服务尚未初始化"));
            return;
        }
        if (!m_templateList->currentItem()) {
            m_hintLabel->setText(tr("请选择模板"));
            return;
        }
        if (m_templateList->currentRow() != 0) {
            QMessageBox::information(this, tr("暂不支持"), tr("所选模板仍在搭建，当前仅支持每日运行概览"));
            return;
        }

        const QDate date = m_dateEdit->date();
        const QString defaultName = tr("每日运行概览_%1.pdf").arg(date.toString("yyyyMMdd"));
        const QString suggestedDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        const QString outputPath = QFileDialog::getSaveFileName(this,
                                                               tr("导出 PDF"),
                                                               suggestedDir + QLatin1Char('/') + defaultName,
                                                               tr("PDF 文件 (*.pdf)"));
        if (outputPath.isEmpty()) {
            return;
        }

        m_hintLabel->setText(tr("正在生成 %1 ...").arg(m_templateList->currentItem()->text()));
        m_service->generateDailyOverview(date, outputPath);
    });
}

void ReportPage::applyConfig(const ReportService::ReportConfig &config)
{
    if (!m_scheduleEdit || !m_ownerEdit || !m_archiveSpin) {
        return;
    }

    m_scheduleEdit->setText(config.schedule);
    m_ownerEdit->setText(config.owners.join(QStringLiteral(", ")));
    m_archiveSpin->setValue(config.archiveDays);
}

QStringList ReportPage::parseOwners(const QString &text) const
{
    const QRegularExpression splitter(QStringLiteral("[\\s,;，、]+"));
    QStringList owners;
    for (const auto &part : text.split(splitter, Qt::SkipEmptyParts)) {
        owners.append(part.trimmed());
    }
    return owners;
}
