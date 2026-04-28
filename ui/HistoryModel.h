#pragma once

#include "DataLoader.h"

#include <QAbstractTableModel>
#include <QVector>

namespace ui {

/**
 * @brief Qt model that exposes the scraped transactions to a QTableView.
 *
 * Used by the History tab. The raw QVector<Transaction> is wrapped in a
 * QSortFilterProxyModel inside MainWindow so column-header clicks sort
 * the table without us having to re-sort the underlying vector.
 *
 * Two display roles are returned:
 *   - Qt::DisplayRole — formatted strings shown to the user
 *                        (date "yyyy-MM-dd hh:mm", currency "$xx.xx").
 *   - Qt::EditRole    — raw typed values (QDateTime, double) used for
 *                        correct sort order on the proxy.
 */
class HistoryModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    /// Column indices, kept as an enum so MainWindow can refer to them by name.
    enum Column { Date = 0, Shop = 1, Amount = 2, Balance = 3, ColumnCount = 4 };

    explicit HistoryModel(QObject* parent = nullptr);

    /// Replace the entire table contents and notify any attached views.
    void setTransactions(const QVector<Transaction>& rows);

    int      rowCount(const QModelIndex& parent = {}) const override;
    int      columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    QVector<Transaction> rows_;
};

}
