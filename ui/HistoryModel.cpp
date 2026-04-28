#include "HistoryModel.h"

#include <QLocale>

namespace ui {

HistoryModel::HistoryModel(QObject* parent)
    : QAbstractTableModel(parent) {}

void HistoryModel::setTransactions(const QVector<Transaction>& rows)
{
    beginResetModel();
    rows_ = rows;
    endResetModel();
}

int HistoryModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(rows_.size());
}

int HistoryModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant HistoryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rows_.size())
        return {};

    const Transaction& t = rows_.at(index.row());

    // DisplayRole — what the user sees in the cell. Formatted strings.
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Date:    return t.postDate.toString("yyyy-MM-dd  hh:mm");
        case Shop:    return t.location;
        case Amount:  return QStringLiteral("$%1").arg(QLocale::system().toString(t.amount, 'f', 2));
        case Balance: return QStringLiteral("$%1").arg(QLocale::system().toString(t.balance, 'f', 2));
        }
    }

    // EditRole — raw typed values so the QSortFilterProxyModel sorts by
    // numeric / date order instead of by the string "$1.50" < "$10.00".
    if (role == Qt::EditRole) {
        switch (index.column()) {
        case Date:    return t.postDate;
        case Shop:    return t.location;
        case Amount:  return t.amount;
        case Balance: return t.balance;
        }
    }

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == Amount || index.column() == Balance)
            return int(Qt::AlignRight | Qt::AlignVCenter);
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    }

    return {};
}

QVariant HistoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case Date:    return QStringLiteral("Date");
    case Shop:    return QStringLiteral("Shop");
    case Amount:  return QStringLiteral("Amount");
    case Balance: return QStringLiteral("Balance");
    }
    return {};
}

}
