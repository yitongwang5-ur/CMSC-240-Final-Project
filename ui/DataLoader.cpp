#include "DataLoader.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <fstream>
#include <nlohmann/json.hpp>

namespace ui {

using nlohmann::json;

/**
 * @brief Locate the project root (the directory containing ``jsons/`` and
 *        the project ``Makefile``) by walking up from the running binary.
 *
 * The depth varies by deployment:
 *   - bare binary  : ui/build/dining_ui                       → 2 cd-up's
 *   - .app bundle  : ui/build/dining_ui.app/Contents/MacOS/.. → 4 cd-up's
 *
 * Six levels of search comfortably covers either layout, plus future
 * relocations under build/ subfolders.
 */
QString repoRootPath()
{
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 6; ++i) {
        if (dir.exists("jsons") && dir.exists("Makefile"))
            return dir.absolutePath();
        if (!dir.cdUp()) break;
    }
    // Fallback: assume the user ran the binary from the repo root.
    return QDir::currentPath();
}

QString historyPath()
{
    return QDir(repoRootPath()).filePath("jsons/history.json");
}

QString statusPath()
{
    return QDir(repoRootPath()).filePath("jsons/status.json");
}

/// Read a UTF-8 text file fully into a QString, or set *err with a friendly message.
static QString readFile(const QString& path, QString* err)
{
    QFile f(path);
    if (!f.exists()) {
        *err = QStringLiteral("Missing file: %1").arg(path);
        return {};
    }
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        *err = QStringLiteral("Cannot read: %1").arg(path);
        return {};
    }
    return QString::fromUtf8(f.readAll());
}

/// Parse the scraper's "MM/dd/yyyy HH:mm:ss" timestamp into a QDateTime.
static QDateTime parsePostDate(const std::string& raw)
{
    return QDateTime::fromString(QString::fromStdString(raw), "MM/dd/yyyy HH:mm:ss");
}

DataBundle loadAll()
{
    DataBundle bundle;

    // ── history.json — required ──────────────────────────────────────────
    // If this file is missing or unparseable the dashboard cannot show any
    // data, so populate bundle.error and return early.
    QString histErr;
    const QString histText = readFile(historyPath(), &histErr);
    if (!histErr.isEmpty()) {
        bundle.error = histErr;
        return bundle;
    }

    try {
        const auto h = json::parse(histText.toStdString());
        if (h.contains("timelineData")) {
            for (const auto& row : h.at("timelineData")) {
                if (!row.is_array() || row.size() < 4) continue;
                Transaction t;
                t.postDate = parsePostDate(row.at(0).get<std::string>());
                t.location = QString::fromStdString(row.at(1).get<std::string>());
                t.amount = row.at(2).get<double>();
                t.balance = row.at(3).get<double>();
                bundle.transactions.push_back(t);
            }
        }
        if (h.contains("balances")) {
            const auto& b = h.at("balances");
            bundle.status.beginningBalance =
                b.value("beginning_balance", 0.0);
            bundle.status.endingBalance =
                b.value("ending_balance", 0.0);
        }
    } catch (const std::exception& e) {
        bundle.error = QStringLiteral("Parse error in history.json: %1")
                           .arg(QString::fromUtf8(e.what()));
        return bundle;
    }

    // ── status.json — optional ──────────────────────────────────────────
    // If absent we still render the dashboard using only history.json data;
    // pace classification falls back to "on_track" and recommendations
    // appear as zero, but no error is surfaced.
    QString statErr;
    const QString statText = readFile(statusPath(), &statErr);
    if (statErr.isEmpty()) {
        try {
            const auto s = json::parse(statText.toStdString());
            auto& st = bundle.status;
            st.classification     = QString::fromStdString(s.value("classification", std::string("on_track")));
            st.beginningBalance   = s.value("beginning_balance",   st.beginningBalance);
            st.endingBalance      = s.value("ending_balance",      st.endingBalance);
            st.spent              = s.value("spent",               st.beginningBalance - st.endingBalance);
            st.expectedSpent      = s.value("expected_spent",      0.0);
            st.expectedBalance    = s.value("expected_balance",    0.0);
            st.deviationRatio     = s.value("deviation_ratio",     0.0);
            st.tolerance          = s.value("tolerance",           0.10);
            st.daysElapsed        = s.value("days_elapsed",        0);
            st.daysTotal          = s.value("days_total",          1);
            st.today              = QString::fromStdString(s.value("today", std::string{}));
            st.semesterStart      = QString::fromStdString(s.value("semester_start", std::string{}));
            st.semesterEnd        = QString::fromStdString(s.value("semester_end", std::string{}));
            st.recommendedDailyUsage  = s.value("recommended_daily_usage",  0.0);
            st.recommendedWeeklyUsage = s.value("recommended_weekly_usage", 0.0);

            QFileInfo fi(statusPath());
            bundle.statusFileMtime = fi.lastModified();
        } catch (const std::exception&) {
        }
    } else {
        bundle.status.spent = bundle.status.beginningBalance - bundle.status.endingBalance;
        bundle.status.classification = "on_track";
    }

    return bundle;
}

}
