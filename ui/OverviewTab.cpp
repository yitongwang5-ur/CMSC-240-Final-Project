#include "OverviewTab.h"
#include "Palette.h"

#include <QDate>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QMap>
#include <QVBoxLayout>

namespace ui {

OverviewTab::Card OverviewTab::makeCard(const QString& titleText)
{
    auto* card = new QFrame(this);
    card->setObjectName("Card");
    card->setMinimumHeight(120);

    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(20, 18, 20, 18);
    lay->setSpacing(6);

    Card c;
    c.title = new QLabel(titleText, card);
    c.title->setObjectName("CardTitle");
    c.value = new QLabel("—", card);
    c.value->setObjectName("CardValue");
    c.sub = new QLabel(QString(), card);
    c.sub->setObjectName("CardSubtext");
    c.sub->setWordWrap(true);

    lay->addWidget(c.title);
    lay->addWidget(c.value);
    lay->addWidget(c.sub);
    lay->addStretch();

    const int idx = grid_->count();
    grid_->addWidget(card, idx / 3, idx % 3);
    return c;
}

OverviewTab::OverviewTab(QWidget* parent)
    : QWidget(parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(20, 20, 20, 20);
    outer->setSpacing(16);

    auto* gridHost = new QWidget(this);
    grid_ = new QGridLayout(gridHost);
    grid_->setHorizontalSpacing(16);
    grid_->setVerticalSpacing(16);

    avgDay_   = makeCard("Average $ / day");
    avgWeek_  = makeCard("Average $ / week");
    avgMonth_ = makeCard("Average $ / month");
    recDay_   = makeCard("Recommended $ / day");
    recWeek_  = makeCard("Recommended $ / week");
    pace_     = makeCard("Pace");

    outer->addWidget(gridHost);
    outer->addStretch();
}

static QString fmtMoney(double v)
{
    return QString("$%1").arg(v, 0, 'f', 2);
}

void OverviewTab::setData(const DataBundle& data)
{
    const auto& tx = data.transactions;
    const auto& s = data.status;

    // Walk the transactions once: total spend, per-day totals (for charts),
    // and the date range covered by the data (for the "average" denominator).
    double totalSpent = 0.0;
    QMap<QDate, double> perDay;
    QDate firstDay, lastDay;
    for (const Transaction& t : tx) {
        totalSpent += t.amount;
        const QDate d = t.postDate.date();
        if (!d.isValid()) continue;
        perDay[d] += t.amount;
        if (!firstDay.isValid() || d < firstDay) firstDay = d;
        if (!lastDay.isValid()  || d > lastDay)  lastDay  = d;
    }

    // Prefer the actual data range for the "$/day" denominator. Fall back to
    // the analyzer's daysElapsed if we somehow have status without rows.
    const int spanDays = (firstDay.isValid() && lastDay.isValid())
                             ? std::max(1, static_cast<int>(firstDay.daysTo(lastDay)) + 1)
                             : std::max(1, s.daysElapsed);

    const double avgDay = (spanDays > 0) ? totalSpent / spanDays : 0.0;
    const double avgWeek = avgDay * 7.0;
    const double avgMonth = avgDay * 30.0;

    avgDay_.value->setText(fmtMoney(avgDay));
    avgDay_.sub->setText(QString("Across %1 day(s) of activity").arg(spanDays));

    avgWeek_.value->setText(fmtMoney(avgWeek));
    avgWeek_.sub->setText("Daily average × 7");

    avgMonth_.value->setText(fmtMoney(avgMonth));
    avgMonth_.sub->setText("Daily average × 30");

    recDay_.value->setText(fmtMoney(s.recommendedDailyUsage));
    recDay_.sub->setText(QString("To finish on %1").arg(s.semesterEnd));

    recWeek_.value->setText(fmtMoney(s.recommendedWeeklyUsage));
    recWeek_.sub->setText("Recommended daily × 7");

    QString paceText, paceColor, paceSub;
    if (s.classification == "overspending") {
        paceText = "Overspending";
        paceColor = palette::red.name();
        paceSub = QString("$%1 ahead of even pace").arg(qAbs(s.expectedBalance - s.endingBalance), 0, 'f', 2);
    } else if (s.classification == "underspending") {
        paceText = "Underspending";
        paceColor = palette::orange.name();
        paceSub = QString("$%1 behind even pace").arg(qAbs(s.endingBalance - s.expectedBalance), 0, 'f', 2);
    } else {
        paceText = "On track";
        paceColor = palette::green.name();
        paceSub = QString("Within ±%1% of even pace").arg(int(s.tolerance * 100));
    }
    pace_.value->setText(paceText);
    pace_.value->setStyleSheet(QString("color: %1; background: transparent;").arg(paceColor));
    pace_.sub->setText(paceSub);
}

}
