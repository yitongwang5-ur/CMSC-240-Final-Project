#pragma once

#include "DataLoader.h"

#include <QFrame>

class QLabel;

namespace ui {

/**
 * @brief Color-coded banner that summarises the user's spending pace.
 *
 * Sits directly under the header bar in MainWindow. The icon, title, and
 * background tint switch based on Status::classification:
 *   - "overspending"  → red    + ⚠
 *   - "underspending" → orange + ℹ
 *   - "on_track"      → green  + ✓
 *
 * The detail line shows current balance, expected balance, and the next
 * recommended daily-spend target, so the user gets the key numbers without
 * having to switch to the Recommendations tab.
 */
class StatusBanner : public QFrame
{
    Q_OBJECT
public:
    explicit StatusBanner(QWidget* parent = nullptr);

    /// Repaint the banner from a fresh Status snapshot.
    void setStatus(const Status& s);

private:
    QLabel* icon_   = nullptr;
    QLabel* title_  = nullptr;
    QLabel* detail_ = nullptr;
};

}
