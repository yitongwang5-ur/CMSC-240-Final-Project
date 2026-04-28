#include "StatusBanner.h"
#include "Palette.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace ui {

namespace {

/// Visual style for one classification — icon glyph, title text, and three colors.
struct Look {
    QString icon;
    QString title;
    QColor  bg;
    QColor  border;
    QColor  fg;
};

/// Map a classification string from status.json to its visual treatment.
Look lookFor(const QString& cls)
{
    if (cls == "overspending")
        return {QString::fromUtf8("⚠"),    // ⚠
                "Overspending",
                QColor("#fdecea"), palette::red, QColor("#5a1414")};
    if (cls == "underspending")
        return {QString::fromUtf8("ℹ"),    // ℹ
                "Underspending",
                QColor("#fff4e5"), palette::orange, QColor("#5a3a14")};
    return {QString::fromUtf8("✓"),        // ✓
            "On track",
            QColor("#e8f6ec"), palette::green, QColor("#1f4d28")};
}

QString currencyDelta(double v)
{
    const QString sign = v >= 0 ? "+" : "-";
    return QString("%1$%2").arg(sign).arg(qAbs(v), 0, 'f', 2);
}

}

StatusBanner::StatusBanner(QWidget* parent)
    : QFrame(parent)
{
    setObjectName("StatusBanner");
    setFrameShape(QFrame::NoFrame);
    setMinimumHeight(72);

    auto* outer = new QHBoxLayout(this);
    outer->setContentsMargins(20, 14, 20, 14);
    outer->setSpacing(16);

    icon_ = new QLabel(this);
    icon_->setObjectName("StatusIcon");
    icon_->setFixedWidth(32);
    icon_->setAlignment(Qt::AlignCenter);

    auto* textCol = new QVBoxLayout();
    textCol->setSpacing(2);
    title_ = new QLabel(this);
    title_->setObjectName("StatusTitle");
    detail_ = new QLabel(this);
    detail_->setObjectName("StatusDetail");
    detail_->setWordWrap(true);
    textCol->addWidget(title_);
    textCol->addWidget(detail_);

    outer->addWidget(icon_);
    outer->addLayout(textCol, 1);
}

void StatusBanner::setStatus(const Status& s)
{
    const Look look = lookFor(s.classification);

    // Repaint via a stylesheet override (instead of QPalette) so the banner
    // tint cleanly cascades to its child QLabels — palette would only color
    // the QFrame background and leave label text using the global style.
    setStyleSheet(QString(
        "#StatusBanner { background-color: %1; border: 1px solid %2; border-radius: 8px; }"
        "#StatusBanner QLabel { color: %3; }"
    ).arg(look.bg.name(), look.border.name(), look.fg.name()));

    icon_->setText(look.icon);
    title_->setText(look.title);

    const double behindByDollars = s.expectedBalance - s.endingBalance;
    const QString balanceLine =
        QString("Current $%1  •  Expected $%2  •  %3")
            .arg(s.endingBalance, 0, 'f', 2)
            .arg(s.expectedBalance, 0, 'f', 2)
            .arg(currencyDelta(-behindByDollars));

    QString suggestion;
    if (s.classification == "overspending") {
        suggestion = QString("Slow down — try $%1/day to last through %2.")
                         .arg(s.recommendedDailyUsage, 0, 'f', 2)
                         .arg(s.semesterEnd);
    } else if (s.classification == "underspending") {
        suggestion = QString("You can comfortably spend up to $%1/day through %2.")
                         .arg(s.recommendedDailyUsage, 0, 'f', 2)
                         .arg(s.semesterEnd);
    } else {
        suggestion = QString("Keep pace at about $%1/day through %2.")
                         .arg(s.recommendedDailyUsage, 0, 'f', 2)
                         .arg(s.semesterEnd);
    }

    detail_->setText(balanceLine + "    " + suggestion);
}

}
