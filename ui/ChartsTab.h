#pragma once

#include "DataLoader.h"

#include <QWidget>

class QChartView;

namespace ui {

/**
 * @brief Third tab — three native QtCharts that mirror vizualize.py's PNGs.
 *
 *   - **Balance over time**  : actual balance (blue line) + ideal even-pace
 *                              (orange dashed line) on a date axis.
 *   - **Daily spending**     : per-day bars + a horizontal "even" reference.
 *   - **Spending by shop**   : two bar series (visits, total spent) sharing
 *                              a category axis of shop names.
 *
 * Stacked vertically inside a QScrollArea so the charts stay legible at any
 * window size — the user just scrolls if the window is short.
 */
class ChartsTab : public QWidget
{
    Q_OBJECT
public:
    explicit ChartsTab(QWidget* parent = nullptr);

    /// Rebuild all three charts from a fresh DataBundle.
    void setData(const DataBundle& data);

private:
    QChartView* balanceView_ = nullptr;
    QChartView* dailyView_   = nullptr;
    QChartView* shopsView_   = nullptr;
};

}
