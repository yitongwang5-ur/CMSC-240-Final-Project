#pragma once

#include "DataLoader.h"

#include <QMainWindow>

class QLabel;
class QProgressBar;
class QPushButton;
class QSortFilterProxyModel;
class QSystemTrayIcon;
class QTableView;
class QTimer;

namespace ui {

class StatusBanner;
class HistoryModel;
class OverviewTab;
class ChartsTab;
class RecommendationsTab;

/**
 * @brief Top-level dashboard window — owns every other UI piece.
 *
 * Layout (top to bottom):
 *   1. **Title bar**     — UR-red banner with beginning / current / spent
 *                          balances and a semester-progress bar.
 *   2. **Status banner** — color-coded pace summary (over / on / under).
 *   3. **Tabs**          — Overview · History · Charts · Recommendations.
 *   4. **Footer**        — Refresh + Run-scrape buttons and last-update time.
 *
 * Side responsibilities:
 *   - **System tray**       (QSystemTrayIcon) — menu-bar entry with Show /
 *                            Refresh / Test-notification / Quit.
 *   - **Periodic poll**     (QTimer, 5 min) — re-reads jsons/status.json
 *                            so external pipeline updates surface live.
 *   - **Notification dedup** (maybeNotify) — only fires a banner when the
 *                            classification actually changes (or on first
 *                            load if the user is already over/under pace).
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    /// Reload both JSONs from disk and repaint everything.
    void onRefresh();
    /// Shell out to ``make run`` to re-scrape + analyze + visualize.
    void onRunScrape();

private:
    void buildUi();                                        ///< Construct widgets + layout.
    void setupTray();                                      ///< Create the menu-bar tray icon + context menu.
    void applyData(const DataBundle& data);                ///< Push fresh data into every child widget.
    void showErrorPane(const QString& message);            ///< Friendly error UX when JSONs are missing/bad.
    void maybeNotify(const Status& current);               ///< Fire a tray banner if classification changed.

    QWidget* centralRoot_ = nullptr;

    QLabel* beginValue_ = nullptr;
    QLabel* currentValue_ = nullptr;
    QLabel* spentValue_ = nullptr;
    QLabel* dayLabel_ = nullptr;
    QProgressBar* progress_ = nullptr;

    StatusBanner* banner_ = nullptr;

    OverviewTab* overview_ = nullptr;
    ChartsTab* charts_ = nullptr;
    RecommendationsTab* recommendations_ = nullptr;

    HistoryModel* historyModel_ = nullptr;
    QSortFilterProxyModel* historyProxy_ = nullptr;
    QTableView* historyTable_ = nullptr;

    QPushButton* refreshBtn_ = nullptr;
    QPushButton* scrapeBtn_ = nullptr;
    QLabel* footerStatus_ = nullptr;

    QSystemTrayIcon* tray_      = nullptr;  ///< macOS Notification Center / Windows toast bridge.
    QTimer*          pollTimer_ = nullptr;  ///< 5-minute periodic onRefresh().

    bool    firstLoad_             = true;  ///< Differentiates "show alert on launch" from later refreshes.
    QString lastClassification_;            ///< For change detection in maybeNotify().
    bool    hasLastClassification_ = false; ///< True once a classification has been observed at least once.
};

}
