#include "ui/components/queryform.h"

#include <ElaComboBox.h>
#include <ElaLineEdit.h>
#include <ElaPushButton.h>
#include <ElaText.h>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QGridLayout>
#include <QList>
#include <QPalette>
#include <QSizePolicy>
#include <QVariant>
#include <QVBoxLayout>

QueryForm::QueryForm(QWidget *parent)
    : QWidget(parent)
{
    buildLayout();
    connectSignals();
    resetForm();
    setVehicleIdEnabled(true);
    setBoundsEnabled(false);
}

QueryParameters QueryForm::parameters() const
{
    QueryParameters params;
    params.vehicleId = m_vehicleInput->text();
    params.queryType = m_typeCombo->currentText();
    params.type = static_cast<QueryType>(m_typeCombo->currentData().toInt());
    params.longitudeMin = m_lonMin->text();
    params.longitudeMax = m_lonMax->text();
    params.latitudeMin = m_latMin->text();
    params.latitudeMax = m_latMax->text();
    params.startTime = m_startTime->dateTime();
    params.endTime = m_endTime->dateTime();
    return params;
}

void QueryForm::buildLayout()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    auto *title = new ElaText(tr("轨迹检索"), this);
    title->setTextStyle(ElaTextType::Title);
    title->setTextPixelSize(22);
    layout->addWidget(title);

    auto *description = new ElaText(tr("根据车辆、区域与时间条件快速定位目标轨迹"), this);
    description->setTextStyle(ElaTextType::Caption);
    description->setTextPixelSize(13);
    auto descPalette = description->palette();
    descPalette.setColor(QPalette::WindowText, descPalette.color(QPalette::WindowText).lighter(140));
    description->setPalette(descPalette);
    layout->addWidget(description);

    auto makeLabel = [this]() {
        auto *label = new ElaText(QString(), this);
        label->setTextStyle(ElaTextType::BodyStrong);
        label->setTextPixelSize(15);
        auto palette = label->palette();
        palette.setColor(QPalette::WindowText, palette.color(QPalette::WindowText).darker(120));
        label->setPalette(palette);
        return label;
    };

    m_vehicleInput = new ElaLineEdit(this);
    m_vehicleInput->setPlaceholderText(tr("车辆编号"));
    m_vehicleInput->setClearButtonEnabled(true);
    m_vehicleInput->setMinimumHeight(36);

    m_typeCombo = new ElaComboBox(this);
    m_typeCombo->addItem(tr("按车辆"), static_cast<int>(QueryType::Trajectory));
    m_typeCombo->addItem(tr("按区域"), static_cast<int>(QueryType::Area));
    m_typeCombo->addItem(tr("按时间"), static_cast<int>(QueryType::Trajectory));
    m_typeCombo->setEditable(false);
    m_typeCombo->setMinimumHeight(36);

    m_lonMin = new ElaLineEdit(this);
    m_lonMin->setPlaceholderText(tr("经度下限"));
    m_lonMin->setClearButtonEnabled(true);
    m_lonMin->setMinimumHeight(36);
    m_lonMax = new ElaLineEdit(this);
    m_lonMax->setPlaceholderText(tr("经度上限"));
    m_lonMax->setClearButtonEnabled(true);
    m_lonMax->setMinimumHeight(36);
    m_latMin = new ElaLineEdit(this);
    m_latMin->setPlaceholderText(tr("纬度下限"));
    m_latMin->setClearButtonEnabled(true);
    m_latMin->setMinimumHeight(36);
    m_latMax = new ElaLineEdit(this);
    m_latMax->setPlaceholderText(tr("纬度上限"));
    m_latMax->setClearButtonEnabled(true);
    m_latMax->setMinimumHeight(36);

    m_startTime = new QDateTimeEdit(this);
    m_startTime->setCalendarPopup(true);
    m_startTime->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm"));
    m_startTime->setMinimumHeight(36);
    m_endTime = new QDateTimeEdit(this);
    m_endTime->setCalendarPopup(true);
    m_endTime->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm"));
    m_endTime->setMinimumHeight(36);

    m_submitButton = new ElaPushButton(tr("执行检索"), this);
    m_submitButton->setMinimumHeight(40);
    m_submitButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_moreButton = new ElaPushButton(tr("更多选项"), this);
    m_moreButton->setMinimumHeight(36);
    m_moreButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto *grid = new QGridLayout();
    grid->setContentsMargins(0, 12, 0, 0);
    grid->setHorizontalSpacing(18);
    grid->setVerticalSpacing(16);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);
    grid->setColumnStretch(3, 0);

    auto *typeLabel = makeLabel();
    typeLabel->setText(tr("检索类型"));
    grid->addWidget(typeLabel, 0, 0);
    grid->addWidget(m_typeCombo, 0, 1, 1, 3);

    auto *vehicleLabel = makeLabel();
    vehicleLabel->setText(tr("车辆信息"));
    grid->addWidget(vehicleLabel, 1, 0);
    grid->addWidget(m_vehicleInput, 1, 1, 1, 3);

    auto *lonLabel = makeLabel();
    lonLabel->setText(tr("经度范围"));
    grid->addWidget(lonLabel, 2, 0);
    grid->addWidget(m_lonMin, 2, 1);
    grid->addWidget(m_lonMax, 2, 2);

    auto *latLabel = makeLabel();
    latLabel->setText(tr("纬度范围"));
    grid->addWidget(latLabel, 3, 0);
    grid->addWidget(m_latMin, 3, 1);
    grid->addWidget(m_latMax, 3, 2);

    auto *timeLabel = makeLabel();
    timeLabel->setText(tr("时间区间"));
    grid->addWidget(timeLabel, 4, 0);
    grid->addWidget(m_startTime, 4, 1);
    grid->addWidget(m_endTime, 4, 2);
    grid->addWidget(m_moreButton, 4, 3);

    layout->addLayout(grid);
    layout->addSpacing(12);
    layout->addWidget(m_submitButton);
    layout->addStretch(1);
}

void QueryForm::connectSignals()
{
    connect(m_submitButton, &ElaPushButton::clicked, this, [this]() {
        emit querySubmitted(parameters());
    });

    connect(m_moreButton, &ElaPushButton::clicked, this, [this]() {
        emit requestAdvancedOptions();
    });

    connect(m_typeCombo, &ElaComboBox::currentIndexChanged, this, [this](int index) {
        const auto type = static_cast<QueryType>(m_typeCombo->itemData(index).toInt());
        emit queryTypeChanged(type);
        const bool needsVehicle = (type == QueryType::Trajectory || type == QueryType::Point);
        const bool needsBounds = (type == QueryType::Area || type == QueryType::Adjacency);
        setVehicleIdEnabled(needsVehicle);
        setBoundsEnabled(needsBounds);
    });
}

void QueryForm::setVehicleIdEnabled(bool enabled)
{
    if (m_vehicleInput) {
        m_vehicleInput->setEnabled(enabled);
    }
}

void QueryForm::setBoundsEnabled(bool enabled)
{
    const QList<ElaLineEdit*> edits{m_lonMin, m_lonMax, m_latMin, m_latMax};
    for (auto *edit : edits) {
        if (!edit) {
            continue;
        }
        edit->setEnabled(enabled);
    }
}

void QueryForm::resetForm()
{
    if (m_vehicleInput) {
        m_vehicleInput->clear();
    }
    for (auto *edit : {m_lonMin, m_lonMax, m_latMin, m_latMax}) {
        if (edit) {
            edit->clear();
        }
    }
    const auto now = QDateTime::currentDateTime();
    if (m_startTime) {
        m_startTime->setDateTime(now.addSecs(-3600));
    }
    if (m_endTime) {
        m_endTime->setDateTime(now);
    }
}
