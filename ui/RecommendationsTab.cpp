#include "RecommendationsTab.h"
#include "Palette.h"

#include <QDate>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMap>
#include <QVBoxLayout>
#include <algorithm>

namespace ui {

namespace {

QFrame* makeCard(QWidget* parent, const QString& title, QLabel** valueOut)
{
    auto* card = new QFrame(parent);
    card->setObjectName("Card");
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(20, 18, 20, 18);
    lay->setSpacing(6);

    auto* t = new QLabel(title, card);
    t->setObjectName("CardTitle");
    auto* v = new QLabel("—", card);
    v->setObjectName("CardValue");
    lay->addWidget(t);
    lay->addWidget(v);
    lay->addStretch();
    *valueOut = v;
    return card;
}

}

RecommendationsTab::RecommendationsTab(QWidget* parent)
    : QWidget(parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(20, 20, 20, 20);
    outer->setSpacing(16);

    headline_ = new QLabel(this);
    headline_->setObjectName("CardTitle");
    headline_->setWordWrap(true);
    outer->addWidget(headline_);

    auto* row = new QHBoxLayout();
    row->setSpacing(16);
    row->addWidget(makeCard(this, "Daily target", &dailyTarget_));
    row->addWidget(makeCard(this, "Weekly target", &weeklyTarget_));
    row->addWidget(makeCard(this, "Days remaining", &horizon_));
    outer->addLayout(row);

    auto* shopsCard = new QFrame(this);
    shopsCard->setObjectName("Card");
    auto* shopsLay = new QVBoxLayout(shopsCard);
    shopsLay->setContentsMargins(20, 18, 20, 18);
    shopsLay->setSpacing(8);
    auto* shopsTitle = new QLabel("Where your money is going", shopsCard);
    shopsTitle->setObjectName("CardTitle");
    shopList_ = new QListWidget(shopsCard);
    shopList_->setFrameShape(QFrame::NoFrame);
    shopList_->setSelectionMode(QAbstractItemView::NoSelection);
    shopList_->setFocusPolicy(Qt::NoFocus);
    shopsLay->addWidget(shopsTitle);
    shopsLay->addWidget(shopList_, 1);
    outer->addWidget(shopsCard, 1);

    alertText_ = new QLabel(this);
    alertText_->setObjectName("CardSubtext");
    alertText_->setWordWrap(true);
    outer->addWidget(alertText_);
}

static QString fmtMoney(double v)
{
    return QString("$%1").arg(v, 0, 'f', 2);
}

void RecommendationsTab::setData(const DataBundle& data)
{
    const auto& s = data.status;

    int daysLeft = std::max(0, s.daysTotal - s.daysElapsed);

    QString headline;
    if (s.classification == "overspending") {
        headline = QString("You're spending faster than your semester pace. "
                           "Try the daily target below to make your $%1 last through %2.")
                       .arg(s.endingBalance, 0, 'f', 2)
                       .arg(s.semesterEnd);
    } else if (s.classification == "underspending") {
        headline = QString("You have more dining dollars than your pace would predict. "
                           "You can comfortably spend up to the daily target below.")
                       .arg(s.endingBalance, 0, 'f', 2);
    } else {
        headline = QString("You're on track. Keep spending around the targets below "
                           "to finish %1 close to $0.").arg(s.semesterEnd);
    }
    headline_->setText(headline);

    dailyTarget_->setText(fmtMoney(s.recommendedDailyUsage));
    weeklyTarget_->setText(fmtMoney(s.recommendedWeeklyUsage));
    horizon_->setText(QString("%1").arg(daysLeft));

    // Aggregate spend + visit count per shop, then sort descending by spend
    // so the user sees their most expensive habits at the top of the list.
    QMap<QString, double> shopSpend;
    QMap<QString, int>    shopVisits;
    for (const Transaction& t : data.transactions) {
        shopSpend[t.location]  += t.amount;
        shopVisits[t.location] += 1;
    }
    QVector<QPair<QString, double>> shops;
    for (auto it = shopSpend.cbegin(); it != shopSpend.cend(); ++it)
        shops.push_back({it.key(), it.value()});
    std::sort(shops.begin(), shops.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    shopList_->clear();
    int top = std::min<int>(5, shops.size());
    for (int i = 0; i < top; ++i) {
        const auto& [name, total] = shops.at(i);
        const int visits = shopVisits.value(name);
        const double avg = visits ? total / visits : 0.0;

        // For the top 2 shops when overspending, suggest a 30% cut per
        // visit. Coarse but actionable; more sophisticated guidance would
        // need to model meal cadence which the scrape doesn't capture.
        QString suggestion;
        if (s.classification == "overspending" && i < 2) {
            suggestion = QString("  →  cap to $%1/visit to ease back to pace")
                             .arg(avg * 0.7, 0, 'f', 2);
        }

        const QString line = QString("%1 — %2 across %3 visit%4 (avg %5)%6")
                                 .arg(name)
                                 .arg(fmtMoney(total))
                                 .arg(visits)
                                 .arg(visits == 1 ? "" : "s")
                                 .arg(fmtMoney(avg))
                                 .arg(suggestion);
        shopList_->addItem(line);
    }
    if (shops.isEmpty())
        shopList_->addItem("No transactions yet — run `make scrape` to load history.");

    QString alert;
    if (s.classification == "overspending") {
        alert = QString("⚠ Alert: at your current pace, you'll hit $0 before %1.").arg(s.semesterEnd);
        alert += QString("  Reduce by ~$%1/day to recover.")
                     .arg(qAbs(s.expectedBalance - s.endingBalance) / std::max(1, daysLeft), 0, 'f', 2);
    } else if (s.classification == "underspending") {
        alert = QString("ℹ Reminder: dining dollars expire on %1. "
                        "You currently have $%2 unspent.")
                    .arg(s.semesterEnd)
                    .arg(s.endingBalance, 0, 'f', 2);
    } else {
        alert = "✓ No alerts — you're tracking close to even pace.";
    }
    alertText_->setText(alert);
}

}
