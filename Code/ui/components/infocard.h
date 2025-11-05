#ifndef UI_COMPONENTS_INFOCARD_H
#define UI_COMPONENTS_INFOCARD_H

#include <QWidget>

class ElaText;
class ElaIconButton;

class InfoCard : public QWidget
{
    Q_OBJECT

public:
    explicit InfoCard(QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setSubtitle(const QString &subtitle);
    void setValue(const QString &value);
    void setTrend(const QString &trendText, bool positive);
    QString title() const;

private:
    ElaText *m_titleLabel{nullptr};
    ElaText *m_subtitleLabel{nullptr};
    ElaText *m_valueLabel{nullptr};
    ElaText *m_trendLabel{nullptr};
};

#endif // UI_COMPONENTS_INFOCARD_H
