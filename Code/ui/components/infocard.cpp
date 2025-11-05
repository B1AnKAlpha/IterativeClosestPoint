#include "ui/components/infocard.h"

#include <QColor>
#include <QHBoxLayout>
#include <QPalette>
#include <QVBoxLayout>

#include "ElaText.h"
#include "ElaTheme.h"

InfoCard::InfoCard(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(140);
    setStyleSheet(QStringLiteral("border-radius: 18px;"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(8);

    m_titleLabel = new ElaText(this);
    m_titleLabel->setTextStyle(ElaTextType::Caption);
    m_titleLabel->setTextPixelSize(14);
    auto titlePalette = m_titleLabel->palette();
    titlePalette.setColor(QPalette::WindowText, titlePalette.color(QPalette::WindowText).lighter(150));
    m_titleLabel->setPalette(titlePalette);

    m_valueLabel = new ElaText(this);
    m_valueLabel->setTextStyle(ElaTextType::TitleLarge);
    m_valueLabel->setTextPixelSize(30);
    auto valueFont = m_valueLabel->font();
    valueFont.setBold(true);
    m_valueLabel->setFont(valueFont);

    m_subtitleLabel = new ElaText(this);
    m_subtitleLabel->setTextStyle(ElaTextType::Body);
    m_subtitleLabel->setTextPixelSize(13);
    auto subtitlePalette = m_subtitleLabel->palette();
    subtitlePalette.setColor(QPalette::WindowText, subtitlePalette.color(QPalette::WindowText).lighter(150));
    m_subtitleLabel->setPalette(subtitlePalette);

    m_trendLabel = new ElaText(this);
    m_trendLabel->setTextStyle(ElaTextType::Caption);
    m_trendLabel->setTextPixelSize(12);

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_valueLabel);
    layout->addWidget(m_subtitleLabel);
    layout->addStretch();
    layout->addWidget(m_trendLabel);

    setObjectName("InfoCard");
}

void InfoCard::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
}

void InfoCard::setSubtitle(const QString &subtitle)
{
    m_subtitleLabel->setText(subtitle);
}

void InfoCard::setValue(const QString &value)
{
    m_valueLabel->setText(value);
}

void InfoCard::setTrend(const QString &trendText, bool positive)
{
    m_trendLabel->setText(trendText);
    QPalette palette = m_trendLabel->palette();
    palette.setColor(QPalette::WindowText, positive ? QColor(46, 204, 113) : QColor(231, 76, 60));
    m_trendLabel->setPalette(palette);
}

QString InfoCard::title() const
{
    return m_titleLabel->text();
}
