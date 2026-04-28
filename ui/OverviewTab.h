#pragma once

#include "DataLoader.h"

#include <QWidget>

class QGridLayout;
class QLabel;

namespace ui {

/**
 * @brief First tab of the dashboard — six "card" stats at a glance.
 *
 * Top row: average $/day, $/week, $/month — computed in-process from the
 * scraped transactions so they stay accurate even if status.json is stale.
 *
 * Bottom row: recommended $/day, recommended $/week, and the pace
 * classification — read straight from status.json (the analyzer already
 * has the math for "to finish on the semester end date").
 */
class OverviewTab : public QWidget
{
    Q_OBJECT
public:
    explicit OverviewTab(QWidget* parent = nullptr);

    /// Recompute and repaint all six cards from a fresh DataBundle.
    void setData(const DataBundle& data);

private:
    /// One stat card — title at top, big value below, subtext underneath.
    struct Card {
        QLabel* title = nullptr;
        QLabel* value = nullptr;
        QLabel* sub   = nullptr;
    };

    /// Build a card widget and place it at the next free slot in the grid.
    Card makeCard(const QString& title);

    QGridLayout* grid_ = nullptr;
    Card avgDay_;
    Card avgWeek_;
    Card avgMonth_;
    Card recDay_;
    Card recWeek_;
    Card pace_;
};

}
