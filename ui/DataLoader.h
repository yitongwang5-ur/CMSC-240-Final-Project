#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>
#include <optional>

/**
 * @file DataLoader.h
 * @brief Reads the JSON files produced by the analyzer + scraper into typed
 *        C++ structs the rest of the UI consumes.
 *
 * The pipeline writes two files into ``jsons/``:
 *   - ``history.json``  — produced by analyzing/analyze.cpp from the scrape;
 *                          contains balances + every transaction.
 *   - ``status.json``   — produced by the same analyzer with the pace
 *                          classification, expected balance, recommendations.
 *
 * loadAll() reads both and returns a DataBundle. Errors (file missing /
 * malformed JSON) are reported via DataBundle::error so the UI can show a
 * friendly fallback pane instead of crashing.
 */
namespace ui {

/// One row of the spending history (matches a row of timelineData in history.json).
struct Transaction {
    QDateTime postDate;  ///< When the charge posted (parsed from "MM/dd/yyyy HH:mm:ss").
    QString   location;  ///< Shop name after analyze.cpp's normalization (Tyler / Cellar / …).
    double    amount;    ///< Dollar amount of this transaction (positive = spent).
    double    balance;   ///< Running balance immediately after this transaction.
};

/**
 * @brief Snapshot of pace + recommendations, mirroring the schema written by
 *        analyze.cpp into ``jsons/status.json``.
 *
 * Field names use camelCase here but map 1:1 to the snake_case keys in the
 * JSON file (see DataLoader.cpp).
 */
struct Status {
    QString classification;          ///< "overspending" | "on_track" | "underspending"
    double  beginningBalance = 0.0;  ///< Dining-dollar allotment at semester start.
    double  endingBalance    = 0.0;  ///< Current remaining balance.
    double  spent            = 0.0;  ///< beginningBalance - endingBalance.
    double  expectedSpent    = 0.0;  ///< Spend expected by today if pacing evenly.
    double  expectedBalance  = 0.0;  ///< Balance expected by today if pacing evenly.
    double  deviationRatio   = 0.0;  ///< (endingBalance - expectedBalance) / beginningBalance.
    double  tolerance        = 0.10; ///< |deviationRatio| <= tolerance → "on_track".
    int     daysElapsed      = 0;    ///< Days elapsed since semesterStart.
    int     daysTotal        = 1;    ///< Total days in the semester window.
    QString today;                   ///< Today's date in "yyyy-MM-dd" form.
    QString semesterStart;           ///< Semester window start, "yyyy-MM-dd".
    QString semesterEnd;             ///< Semester window end, "yyyy-MM-dd".
    double  recommendedDailyUsage  = 0.0;  ///< endingBalance / days remaining.
    double  recommendedWeeklyUsage = 0.0;  ///< recommendedDailyUsage * 7.
};

/// What loadAll() returns — both files' parsed data, plus an error path.
struct DataBundle {
    QVector<Transaction> transactions;     ///< All parsed history rows in chronological order.
    Status               status;           ///< Pace + recommendations.
    QDateTime            statusFileMtime;  ///< status.json mtime (shown in footer "updated …").
    QString              error;            ///< Empty on success; user-facing message otherwise.
    bool ok() const { return error.isEmpty(); }
};

/**
 * @brief Walk up from the executable to find the project root.
 *
 * The .app bundle and the bare binary live at different depths, so we
 * search for a directory that contains both ``jsons/`` and ``Makefile``.
 */
QString repoRootPath();

/// Absolute path to ``<repo>/jsons/history.json``.
QString historyPath();

/// Absolute path to ``<repo>/jsons/status.json``.
QString statusPath();

/**
 * @brief Load history.json + status.json and return a DataBundle.
 *
 * If history.json is missing or malformed, returns a bundle with ``error``
 * set and no transactions — callers should test ``bundle.ok()`` before use.
 * If status.json is missing, the loader synthesises sane defaults so the
 * dashboard still renders a usable Overview tab.
 */
DataBundle loadAll();

}
