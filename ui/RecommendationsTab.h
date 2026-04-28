#pragma once

#include "DataLoader.h"

#include <QWidget>

class QLabel;
class QListWidget;

namespace ui {

/**
 * @brief Fourth tab — actionable spending guidance.
 *
 * Renders four pieces:
 *   1. A one-sentence headline tailored to the current classification
 *      (over/on/under).
 *   2. Three target cards: daily target, weekly target, days remaining.
 *   3. Top shops with per-visit averages, plus a cap suggestion when the
 *      user is overspending ("Tyler — cap to $X/visit to ease back to pace").
 *   4. An alert line that names a concrete corrective action.
 */
class RecommendationsTab : public QWidget
{
    Q_OBJECT
public:
    explicit RecommendationsTab(QWidget* parent = nullptr);

    /// Recompute headline + cards + per-shop list from a fresh DataBundle.
    void setData(const DataBundle& data);

private:
    QLabel*      headline_     = nullptr;
    QLabel*      dailyTarget_  = nullptr;
    QLabel*      weeklyTarget_ = nullptr;
    QLabel*      horizon_      = nullptr;
    QListWidget* shopList_     = nullptr;
    QLabel*      alertText_    = nullptr;
};

}
