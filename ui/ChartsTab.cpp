#include "ChartsTab.h"
#include "Palette.h"

#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QChart>
#include <QChartView>
#include <QDate>
#include <QDateTimeAxis>
#include <QLineSeries>
#include <QMap>
#include <QPainter>
#include <QPen>
#include <QScrollArea>
#include <QValueAxis>
#include <QVBoxLayout>
#include <algorithm>

namespace ui {

namespace {

/// Construct an empty chart view styled to match a card surface.
QChartView* makeChartView(QWidget* parent)
{
    auto* view = new QChartView(parent);
    view->setRenderHint(QPainter::Antialiasing);
    view->setMinimumHeight(280);
    view->setStyleSheet("background-color: #ffffff; border: 1px solid #dcdcdc; border-radius: 8px;");
    return view;
}

/// Apply consistent axis styling (muted labels, dotted grid).
void styleAxis(QAbstractAxis* a)
{
    a->setLabelsColor(palette::muted);
    a->setTitleBrush(palette::muted);
    QPen grid(palette::border);
    grid.setStyle(Qt::DotLine);
    if (auto* va = qobject_cast<QValueAxis*>(a)) {
        va->setGridLineVisible(true);
        va->setGridLinePen(grid);
    } else if (auto* da = qobject_cast<QDateTimeAxis*>(a)) {
        da->setGridLineVisible(true);
        da->setGridLinePen(grid);
    } else if (auto* ca = qobject_cast<QBarCategoryAxis*>(a)) {
        ca->setGridLineVisible(false);
    }
}

}

ChartsTab::ChartsTab(QWidget* parent)
    : QWidget(parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);

    auto* host = new QWidget(scroll);
    auto* hostLay = new QVBoxLayout(host);
    hostLay->setContentsMargins(20, 20, 20, 20);
    hostLay->setSpacing(16);

    balanceView_ = makeChartView(host);
    dailyView_   = makeChartView(host);
    shopsView_   = makeChartView(host);
    shopsView_->setMinimumHeight(360);

    hostLay->addWidget(balanceView_);
    hostLay->addWidget(dailyView_);
    hostLay->addWidget(shopsView_);

    scroll->setWidget(host);
}

/**
 * @brief Balance over time — actual balance line + ideal even-pace line.
 *
 * X-axis is a real QDateTimeAxis (not a category axis) so the line is
 * drawn at correct chronological positions even when transactions are
 * unevenly spaced. The pace line is two endpoints from semester start
 * to semester end, drawn dashed in orange.
 */
static QChart* buildBalanceChart(const DataBundle& data)
{
    auto* chart = new QChart();
    chart->setTitle("Balance over time");
    chart->setBackgroundVisible(false);
    chart->setPlotAreaBackgroundVisible(false);
    chart->setMargins(QMargins(8, 8, 8, 8));
    chart->legend()->setAlignment(Qt::AlignBottom);

    auto* actual = new QLineSeries();
    actual->setName("Actual balance");
    QPen actualPen(palette::blue);
    actualPen.setWidth(2);
    actual->setPen(actualPen);
    for (const Transaction& t : data.transactions) {
        if (!t.postDate.isValid()) continue;
        actual->append(t.postDate.toMSecsSinceEpoch(), t.balance);
    }
    chart->addSeries(actual);

    const auto& s = data.status;
    auto* pace = new QLineSeries();
    pace->setName("Even pace");
    QPen pacePen(palette::orange);
    pacePen.setWidth(2);
    pacePen.setStyle(Qt::DashLine);
    pace->setPen(pacePen);

    QDateTime t0 = QDateTime::fromString(s.semesterStart, "yyyy-MM-dd");
    QDateTime t1 = QDateTime::fromString(s.semesterEnd,   "yyyy-MM-dd");
    if (!t0.isValid()) t0 = QDateTime(QDate(2026,1,12), QTime(0,0));
    if (!t1.isValid()) t1 = QDateTime(QDate(2026,5,3),  QTime(0,0));
    pace->append(t0.toMSecsSinceEpoch(), s.beginningBalance);
    pace->append(t1.toMSecsSinceEpoch(), 0.0);
    chart->addSeries(pace);

    auto* xAxis = new QDateTimeAxis();
    xAxis->setFormat("MMM d");
    xAxis->setTitleText("Date");
    xAxis->setRange(t0, t1);
    styleAxis(xAxis);
    auto* yAxis = new QValueAxis();
    yAxis->setTitleText("Balance ($)");
    yAxis->setRange(0.0, std::max(1.0, s.beginningBalance * 1.05));
    yAxis->setLabelFormat("$%.0f");
    styleAxis(yAxis);
    chart->addAxis(xAxis, Qt::AlignBottom);
    chart->addAxis(yAxis, Qt::AlignLeft);
    actual->attachAxis(xAxis);
    actual->attachAxis(yAxis);
    pace->attachAxis(xAxis);
    pace->attachAxis(yAxis);
    return chart;
}

/**
 * @brief Daily spending bars + horizontal "even pace" reference line.
 *
 * Bars use a QBarCategoryAxis (one slot per day), but a line series cannot
 * be attached to a category axis directly. We work around that by adding a
 * second, hidden QValueAxis spanning [0, N-1] and attaching the reference
 * line to it — both axes share the same plot area, so visually the line
 * sits exactly along the bar centers at the avg-per-day height.
 */
static QChart* buildDailyChart(const DataBundle& data)
{
    auto* chart = new QChart();
    chart->setTitle("Total spending per day");
    chart->setBackgroundVisible(false);
    chart->setPlotAreaBackgroundVisible(false);
    chart->setMargins(QMargins(8, 8, 8, 8));
    chart->legend()->setAlignment(Qt::AlignBottom);

    QMap<QDate, double> perDay;
    for (const Transaction& t : data.transactions) {
        const QDate d = t.postDate.date();
        if (!d.isValid()) continue;
        perDay[d] += t.amount;
    }

    auto* set = new QBarSet("Daily spent");
    set->setColor(palette::green);
    set->setBorderColor(palette::green);
    QStringList categories;
    double maxBar = 0.0;
    for (auto it = perDay.cbegin(); it != perDay.cend(); ++it) {
        *set << it.value();
        categories << it.key().toString("MM/dd");
        maxBar = std::max(maxBar, it.value());
    }
    auto* series = new QBarSeries();
    series->append(set);
    series->setLabelsVisible(false);
    chart->addSeries(series);

    const auto& s = data.status;
    const double avgPerDay = s.beginningBalance / std::max(1, s.daysTotal);
    auto* avg = new QLineSeries();
    avg->setName("Avg $/day if even");
    QPen avgPen(palette::orange);
    avgPen.setWidth(2);
    avgPen.setStyle(Qt::DashLine);
    avg->setPen(avgPen);
    if (!categories.isEmpty()) {
        avg->append(0.0, avgPerDay);
        avg->append(categories.size() - 1, avgPerDay);
    }
    chart->addSeries(avg);

    auto* xAxis = new QBarCategoryAxis();
    xAxis->append(categories);
    xAxis->setTitleText("Day");
    styleAxis(xAxis);
    auto* yAxis = new QValueAxis();
    yAxis->setTitleText("Spent ($)");
    yAxis->setRange(0.0, std::max(maxBar, avgPerDay) * 1.10 + 1.0);
    yAxis->setLabelFormat("$%.0f");
    styleAxis(yAxis);
    chart->addAxis(xAxis, Qt::AlignBottom);
    chart->addAxis(yAxis, Qt::AlignLeft);
    series->attachAxis(xAxis);
    series->attachAxis(yAxis);

    // Hidden value axis ([0 .. N-1]) so the average-line series can be
    // positioned along the bar centers — bars own xAxis (categories), the
    // line owns xVal (numeric). They share the plot area, not the axis.
    auto* xVal = new QValueAxis();
    xVal->setRange(0.0, static_cast<double>(std::max<qsizetype>(0, categories.size() - 1)));
    xVal->setVisible(false);
    chart->addAxis(xVal, Qt::AlignBottom);
    avg->attachAxis(xVal);
    avg->attachAxis(yAxis);
    return chart;
}

/**
 * @brief Per-shop visit counts and dollar totals on a shared category axis.
 *
 * Two QBarSeries on independent y-axes (visits on the left, dollars on the
 * right) so each metric uses the full vertical range without one swamping
 * the other. Sorted descending by total spend so the worst offenders are
 * always on the left.
 */
static QChart* buildShopChart(const DataBundle& data)
{
    auto* chart = new QChart();
    chart->setTitle("Spending by shop");
    chart->setBackgroundVisible(false);
    chart->setPlotAreaBackgroundVisible(false);
    chart->setMargins(QMargins(8, 8, 8, 8));
    chart->legend()->setAlignment(Qt::AlignBottom);

    QMap<QString, double> totals;
    QMap<QString, int> visits;
    for (const Transaction& t : data.transactions) {
        totals[t.location] += t.amount;
        visits[t.location] += 1;
    }
    QList<QString> shops = totals.keys();
    std::sort(shops.begin(), shops.end(), [&](const QString& a, const QString& b) {
        return totals.value(a) > totals.value(b);
    });

    auto* visitsSet = new QBarSet("Visits");
    visitsSet->setColor(palette::purple);
    visitsSet->setBorderColor(palette::purple);
    auto* spentSet = new QBarSet("Total spent ($)");
    spentSet->setColor(palette::cyan);
    spentSet->setBorderColor(palette::cyan);

    QStringList categories;
    double maxVisits = 0, maxSpent = 0;
    for (const QString& s : shops) {
        categories << s;
        const double v = visits.value(s);
        const double t = totals.value(s);
        *visitsSet << v;
        *spentSet  << t;
        maxVisits = std::max(maxVisits, v);
        maxSpent  = std::max(maxSpent, t);
    }

    auto* visitsSeries = new QBarSeries();
    visitsSeries->append(visitsSet);
    auto* spentSeries  = new QBarSeries();
    spentSeries->append(spentSet);
    chart->addSeries(visitsSeries);
    chart->addSeries(spentSeries);

    auto* xAxis = new QBarCategoryAxis();
    xAxis->append(categories);
    xAxis->setLabelsAngle(-30);
    styleAxis(xAxis);

    auto* yLeft = new QValueAxis();
    yLeft->setTitleText("Visits");
    yLeft->setRange(0.0, std::max(1.0, maxVisits * 1.15));
    yLeft->setLabelFormat("%.0f");
    styleAxis(yLeft);

    auto* yRight = new QValueAxis();
    yRight->setTitleText("Total spent ($)");
    yRight->setRange(0.0, std::max(1.0, maxSpent * 1.15));
    yRight->setLabelFormat("$%.0f");
    styleAxis(yRight);

    chart->addAxis(xAxis, Qt::AlignBottom);
    chart->addAxis(yLeft, Qt::AlignLeft);
    chart->addAxis(yRight, Qt::AlignRight);
    visitsSeries->attachAxis(xAxis);
    visitsSeries->attachAxis(yLeft);
    spentSeries->attachAxis(xAxis);
    spentSeries->attachAxis(yRight);
    return chart;
}

void ChartsTab::setData(const DataBundle& data)
{
    balanceView_->setChart(buildBalanceChart(data));
    dailyView_->setChart(buildDailyChart(data));
    shopsView_->setChart(buildShopChart(data));
}

}
