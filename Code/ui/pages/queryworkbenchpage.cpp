#include "ui/pages/queryworkbenchpage.h"

#include "ui/components/queryform.h"

#include <ElaText.h>
#include <QHBoxLayout>
#include <QFrame>
#include <QListWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QSizePolicy>

QueryWorkbenchPage::QueryWorkbenchPage(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("轨迹检索"));
    buildLayout();
    connectSignals();
}

void QueryWorkbenchPage::buildLayout()
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    m_form = new QueryForm(splitter);
    splitter->addWidget(m_form);

    auto *resultPanel = new QWidget(splitter);
    auto *resultLayout = new QVBoxLayout(resultPanel);
    resultLayout->setContentsMargins(24, 24, 24, 24);
    resultLayout->setSpacing(12);

    auto *resultLabel = new ElaText(tr("检索结果"), resultPanel);
    resultLabel->setTextStyle(ElaTextType::Subtitle);
    resultLayout->addWidget(resultLabel);

    m_resultList = new QListWidget(resultPanel);
    m_resultList->setFrameShape(QFrame::NoFrame);
    m_resultList->setSpacing(4);
    resultLayout->addWidget(m_resultList, 1);

    m_emptyLabel = new ElaText(tr("检索结果将在此展示"), resultPanel);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    resultLayout->addWidget(m_emptyLabel);
    resultLayout->addStretch(1);
    updatePlaceholderVisibility();

    resultPanel->setLayout(resultLayout);
    splitter->addWidget(resultPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    layout->addWidget(splitter);
}

void QueryWorkbenchPage::connectSignals()
{
    connect(m_form, &QueryForm::querySubmitted, this, [this](const QueryParameters &params) {
        emit queryTriggered(params);
        if (!params.vehicleId.isEmpty()) {
            emit requestVisualization(params.vehicleId);
        }
    });
}

void QueryWorkbenchPage::appendResult(const QString &label)
{
    if (m_resultList) {
        m_resultList->addItem(label);
        updatePlaceholderVisibility();
    }
}

void QueryWorkbenchPage::clearResults()
{
    if (m_resultList) {
        m_resultList->clear();
        updatePlaceholderVisibility();
    }
}

void QueryWorkbenchPage::updatePlaceholderVisibility()
{
    const bool hasResults = m_resultList && m_resultList->count() > 0;
    if (m_resultList) {
        m_resultList->setVisible(hasResults);
    }
    if (m_emptyLabel) {
        m_emptyLabel->setVisible(!hasResults);
    }
}
