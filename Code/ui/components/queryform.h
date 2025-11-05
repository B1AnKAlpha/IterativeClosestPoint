#ifndef UI_COMPONENTS_QUERYFORM_H
#define UI_COMPONENTS_QUERYFORM_H

#include <QWidget>

#include "common/queryparameters.h"

class ElaComboBox;
class ElaLineEdit;
class ElaPushButton;
class QDateTimeEdit;

class QueryForm : public QWidget
{
    Q_OBJECT

public:
    explicit QueryForm(QWidget *parent = nullptr);

    QueryParameters parameters() const;
    void setVehicleIdEnabled(bool enabled);
    void setBoundsEnabled(bool enabled);
    void resetForm();

signals:
    void querySubmitted(const QueryParameters &parameters);
    void requestAdvancedOptions();
    void queryTypeChanged(QueryType type);

private:
    void buildLayout();
    void connectSignals();

    ElaComboBox *m_typeCombo{nullptr};
    ElaLineEdit *m_vehicleInput{nullptr};
    ElaLineEdit *m_lonMin{nullptr};
    ElaLineEdit *m_lonMax{nullptr};
    ElaLineEdit *m_latMin{nullptr};
    ElaLineEdit *m_latMax{nullptr};
    QDateTimeEdit *m_startTime{nullptr};
    QDateTimeEdit *m_endTime{nullptr};
    ElaPushButton *m_submitButton{nullptr};
    ElaPushButton *m_moreButton{nullptr};
};

#endif // UI_COMPONENTS_QUERYFORM_H
