#include "MainWindow.h"

#include "ChartsTab.h"
#include "DataLoader.h"
#include "HistoryModel.h"
#include "OverviewTab.h"
#include "Palette.h"
#include "RecommendationsTab.h"
#include "StatusBanner.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QTableView>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("UR Dining Dollars");
    resize(1100, 760);
    buildUi();
    setupTray();
    onRefresh();

    // Re-check status.json every 5 minutes. If the classification has changed,
    // maybeNotify() fires a system tray banner. Picks up updates the user runs
    // out-of-band (e.g. `make analyze` from a terminal). Five minutes is short
    // enough to feel responsive but long enough to avoid disk-thrash on laptops.
    pollTimer_ = new QTimer(this);
    pollTimer_->setInterval(5 * 60 * 1000);
    connect(pollTimer_, &QTimer::timeout, this, &MainWindow::onRefresh);
    pollTimer_->start();
}

static QFrame* makeTitleBar(QWidget* parent,
                            QLabel** beginValue, QLabel** currentValue, QLabel** spentValue,
                            QLabel** dayLabel, QProgressBar** progress)
{
    auto* bar = new QFrame(parent);
    bar->setObjectName("TitleBar");
    bar->setAutoFillBackground(true);

    auto* outer = new QVBoxLayout(bar);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto* topRow = new QHBoxLayout();
    topRow->setContentsMargins(20, 14, 20, 6);
    topRow->setSpacing(0);
    auto* title = new QLabel("UR Dining Dollars", bar);
    title->setObjectName("TitleBarLabel");
    auto* subtitle = new QLabel("University of Richmond  •  Spending insights", bar);
    subtitle->setObjectName("TitleBarSubtitle");
    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(0);
    titleCol->setContentsMargins(0, 0, 0, 0);
    titleCol->addWidget(title);
    titleCol->addWidget(subtitle);
    topRow->addLayout(titleCol);
    topRow->addStretch();

    auto makeStat = [&](const QString& label, QLabel** valueOut) {
        auto* col = new QVBoxLayout();
        col->setSpacing(0);
        auto* l = new QLabel(label, bar);
        l->setObjectName("HeaderBalanceLabel");
        l->setAlignment(Qt::AlignRight);
        auto* v = new QLabel("—", bar);
        v->setObjectName("HeaderBalance");
        v->setAlignment(Qt::AlignRight);
        col->addWidget(l);
        col->addWidget(v);
        *valueOut = v;
        return col;
    };

    topRow->addLayout(makeStat("BEGINNING", beginValue));
    topRow->addSpacing(28);
    topRow->addLayout(makeStat("CURRENT", currentValue));
    topRow->addSpacing(28);
    topRow->addLayout(makeStat("SPENT", spentValue));

    auto* progressRow = new QHBoxLayout();
    progressRow->setContentsMargins(20, 0, 20, 12);
    progressRow->setSpacing(12);
    *dayLabel = new QLabel("Day — of —", bar);
    (*dayLabel)->setStyleSheet("color: #f4d8d8; background: transparent; font-size: 10pt;");
    *progress = new QProgressBar(bar);
    (*progress)->setRange(0, 100);
    (*progress)->setValue(0);
    (*progress)->setTextVisible(false);
    progressRow->addWidget(*dayLabel);
    progressRow->addWidget(*progress, 1);

    outer->addLayout(topRow);
    outer->addLayout(progressRow);
    return bar;
}

void MainWindow::buildUi()
{
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* titleBar = makeTitleBar(central, &beginValue_, &currentValue_, &spentValue_,
                                  &dayLabel_, &progress_);
    root->addWidget(titleBar);

    auto* body = new QWidget(central);
    centralRoot_ = body;
    auto* bodyLay = new QVBoxLayout(body);
    bodyLay->setContentsMargins(20, 16, 20, 16);
    bodyLay->setSpacing(14);

    banner_ = new StatusBanner(body);
    bodyLay->addWidget(banner_);

    auto* tabs = new QTabWidget(body);
    overview_ = new OverviewTab(tabs);

    auto* historyHost = new QWidget(tabs);
    auto* historyLay = new QVBoxLayout(historyHost);
    historyLay->setContentsMargins(20, 20, 20, 20);
    historyTable_ = new QTableView(historyHost);
    historyModel_ = new HistoryModel(this);
    historyProxy_ = new QSortFilterProxyModel(this);
    historyProxy_->setSourceModel(historyModel_);
    historyProxy_->setSortRole(Qt::EditRole);
    historyTable_->setModel(historyProxy_);
    historyTable_->setSortingEnabled(true);
    historyTable_->sortByColumn(HistoryModel::Date, Qt::DescendingOrder);
    historyTable_->setAlternatingRowColors(true);
    historyTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    historyTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    historyTable_->verticalHeader()->setVisible(false);
    historyTable_->horizontalHeader()->setStretchLastSection(true);
    historyTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    historyTable_->setShowGrid(false);
    historyLay->addWidget(historyTable_);

    charts_ = new ChartsTab(tabs);
    recommendations_ = new RecommendationsTab(tabs);

    tabs->addTab(overview_, "Overview");
    tabs->addTab(historyHost, "History");
    tabs->addTab(charts_, "Charts");
    tabs->addTab(recommendations_, "Recommendations");
    bodyLay->addWidget(tabs, 1);

    auto* footer = new QHBoxLayout();
    footer->setContentsMargins(0, 0, 0, 0);
    refreshBtn_ = new QPushButton("Refresh", body);
    scrapeBtn_  = new QPushButton("Run scrape", body);
    scrapeBtn_->setObjectName("SecondaryButton");
    footerStatus_ = new QLabel(body);
    footerStatus_->setObjectName("CardSubtext");

    footer->addWidget(refreshBtn_);
    footer->addWidget(scrapeBtn_);
    footer->addStretch();
    footer->addWidget(footerStatus_);
    bodyLay->addLayout(footer);

    root->addWidget(body, 1);
    setCentralWidget(central);

    connect(refreshBtn_, &QPushButton::clicked, this, &MainWindow::onRefresh);
    connect(scrapeBtn_, &QPushButton::clicked, this, &MainWindow::onRunScrape);
}

/**
 * @brief Paint a 64×64 "$" badge in UR red for use as the tray icon.
 *
 * Done programmatically so we don't have to ship a .png resource — the
 * icon is generated once at startup and cached by Qt.
 */
static QIcon makeTrayIcon()
{
    QPixmap pm(64, 64);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(palette::urRed);
    p.setPen(Qt::NoPen);
    p.drawEllipse(2, 2, 60, 60);
    p.setPen(Qt::white);
    QFont f = p.font();
    f.setBold(true);
    f.setPixelSize(40);
    p.setFont(f);
    p.drawText(pm.rect(), Qt::AlignCenter, "$");
    p.end();
    return QIcon(pm);
}

/**
 * @brief Install the menu-bar (tray) icon and its right-click menu.
 *
 * Skipped silently when no system tray exists (headless / offscreen Qt
 * platform). On macOS, notifications only work when the app is launched
 * from a proper .app bundle — see ui/CMakeLists.txt for MACOSX_BUNDLE.
 */
void MainWindow::setupTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;

    tray_ = new QSystemTrayIcon(makeTrayIcon(), this);
    tray_->setToolTip("UR Dining Dollars");

    auto* menu = new QMenu(this);
    auto* showAct    = menu->addAction("Show dashboard");
    auto* refreshAct = menu->addAction("Refresh now");
    auto* testAct    = menu->addAction("Send test notification");
    menu->addSeparator();
    auto* quitAct    = menu->addAction("Quit");

    connect(showAct, &QAction::triggered, this, [this]() {
        showNormal();
        raise();
        activateWindow();
    });
    connect(refreshAct, &QAction::triggered, this, &MainWindow::onRefresh);
    connect(testAct, &QAction::triggered, this, [this]() {
        if (tray_ && QSystemTrayIcon::supportsMessages()) {
            tray_->showMessage("UR Dining Dollars",
                               "Notifications are working. You'll see banners here "
                               "when your spending pace changes.",
                               QSystemTrayIcon::Information, 6000);
        }
    });
    connect(quitAct, &QAction::triggered, qApp, &QCoreApplication::quit);
    connect(tray_, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason r) {
        if (r == QSystemTrayIcon::Trigger || r == QSystemTrayIcon::DoubleClick) {
            showNormal();
            raise();
            activateWindow();
        }
    });

    tray_->setContextMenu(menu);
    tray_->show();
}

/**
 * @brief Decide whether to fire a tray-banner notification.
 *
 * Two cases produce a banner:
 *   1. **First load** — only if the user is already over- or underspending
 *      (we don't ping someone who is on track at launch — that would be
 *      noise).
 *   2. **Subsequent loads** — only when the classification actually
 *      *changed* since the last poll, e.g. on_track → overspending or
 *      overspending → on_track. This is what makes the 5-minute polling
 *      useful without spamming the user.
 *
 * The banner copy and severity are picked per-case so the user gets
 * actionable language ("Slow to $X/day") rather than just a label.
 */
void MainWindow::maybeNotify(const Status& s)
{
    if (!tray_ || !QSystemTrayIcon::supportsMessages())
        return;

    const bool firstSeen     = !hasLastClassification_;
    const bool changed       = hasLastClassification_ && (s.classification != lastClassification_);
    const bool worthAlerting = (s.classification != "on_track");

    QString title, body;
    QSystemTrayIcon::MessageIcon level = QSystemTrayIcon::Information;

    if (firstSeen && worthAlerting) {
        if (s.classification == "overspending") {
            title = "Spending alert — over pace";
            level = QSystemTrayIcon::Warning;
            body = QString("$%1 left, expected $%2.\n"
                           "Try $%3/day to last through %4.")
                       .arg(s.endingBalance, 0, 'f', 2)
                       .arg(s.expectedBalance, 0, 'f', 2)
                       .arg(s.recommendedDailyUsage, 0, 'f', 2)
                       .arg(s.semesterEnd);
        } else {
            title = "Reminder — under pace";
            body = QString("$%1 unspent at day %2/%3.\n"
                           "You can spend up to $%4/day until %5.")
                       .arg(s.endingBalance, 0, 'f', 2)
                       .arg(s.daysElapsed)
                       .arg(s.daysTotal)
                       .arg(s.recommendedDailyUsage, 0, 'f', 2)
                       .arg(s.semesterEnd);
        }
    } else if (changed) {
        if (s.classification == "overspending") {
            title = "Now overspending";
            level = QSystemTrayIcon::Warning;
            body = QString("Pace just shifted past the %1% threshold.\n"
                           "Recommended: $%2/day.")
                       .arg(int(s.tolerance * 100))
                       .arg(s.recommendedDailyUsage, 0, 'f', 2);
        } else if (s.classification == "underspending") {
            title = "Now underspending";
            body = QString("You can comfortably spend up to $%1/day.")
                       .arg(s.recommendedDailyUsage, 0, 'f', 2);
        } else {
            title = "Back on track";
            body = QString("Pace is within ±%1% of even.")
                       .arg(int(s.tolerance * 100));
        }
    }

    // Empty title = nothing to alert on (e.g. first load and on_track).
    if (!title.isEmpty())
        tray_->showMessage(title, body, level, 8000);

    // Always remember the classification, even when we suppressed the alert,
    // so a later transition is correctly detected as a change.
    lastClassification_    = s.classification;
    hasLastClassification_ = true;
}

static QString fmtMoney(double v)
{
    return QString("$%1").arg(v, 0, 'f', 2);
}

void MainWindow::applyData(const DataBundle& data)
{
    const auto& s = data.status;

    beginValue_->setText(fmtMoney(s.beginningBalance));
    currentValue_->setText(fmtMoney(s.endingBalance));
    spentValue_->setText(fmtMoney(s.spent));

    const int total = std::max(1, s.daysTotal);
    const int elapsed = std::max(0, std::min(total, s.daysElapsed));
    progress_->setRange(0, total);
    progress_->setValue(elapsed);
    dayLabel_->setText(QString("Day %1 of %2").arg(elapsed).arg(total));

    banner_->setStatus(s);
    overview_->setData(data);
    charts_->setData(data);
    recommendations_->setData(data);
    historyModel_->setTransactions(data.transactions);
    historyTable_->resizeColumnsToContents();
    historyTable_->horizontalHeader()->setStretchLastSection(true);

    if (data.statusFileMtime.isValid()) {
        footerStatus_->setText(QString("status.json updated %1")
                                   .arg(data.statusFileMtime.toString("yyyy-MM-dd hh:mm")));
    } else {
        footerStatus_->setText("status.json not found — run `make analyze`");
    }

    maybeNotify(s);
    firstLoad_ = false;
}

void MainWindow::showErrorPane(const QString& message)
{
    footerStatus_->setText(message);
    Status empty;
    empty.classification = "on_track";
    empty.semesterEnd = "—";
    empty.recommendedDailyUsage = 0.0;
    banner_->setStatus(empty);

    QMessageBox box(this);
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle("No data yet");
    box.setText("This dashboard needs scraped + analyzed data.");
    box.setInformativeText(message + "\n\nRun the scrape + analyze pipeline to populate the dashboard.");
    auto* runScrape = box.addButton("Run scrape", QMessageBox::ActionRole);
    box.addButton("Dismiss", QMessageBox::RejectRole);
    box.exec();
    if (box.clickedButton() == runScrape)
        onRunScrape();
}

void MainWindow::onRefresh()
{
    DataBundle data = loadAll();
    if (!data.ok()) {
        showErrorPane(data.error);
        return;
    }
    applyData(data);
}

/**
 * @brief Run the full scrape + analyze + visualize pipeline, then refresh.
 *
 * Blocks the UI for the duration of the subprocess. The scraper prompts
 * for NetID + password on its own stdin, so the user sees the credential
 * prompt in whatever terminal they launched the app from. Channels are
 * forwarded so the prompt is visible in real time.
 */
void MainWindow::onRunScrape()
{
    refreshBtn_->setEnabled(false);
    scrapeBtn_->setEnabled(false);
    footerStatus_->setText("Running `make run` — check the terminal for credentials prompt…");
    // Force the footer text to repaint before we block on QProcess.
    QApplication::processEvents();

    QProcess proc;
    proc.setProcessChannelMode(QProcess::ForwardedChannels);
    proc.setWorkingDirectory(repoRootPath());
    proc.start("make", {"run"});
    proc.waitForFinished(-1);

    refreshBtn_->setEnabled(true);
    scrapeBtn_->setEnabled(true);
    onRefresh();
}

}
