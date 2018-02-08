#ifndef PALETTEM_H
#define PALETTEM_H
#include <QObject>
#include <QModelIndex>
#include <QAbstractItemModel>
#include <QAbstractTableModel>

class PaletteM : public QAbstractTableModel
{
public:
    QColor pal[256];
    int pal_c;

    explicit PaletteM(QObject *p);

    int rowCount(const QModelIndex &) const { return 32; }
    int columnCount(const QModelIndex &) const { return 8; }

    int getidx(const QModelIndex &i) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    Qt::ItemFlags flags(const QModelIndex &) const {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled
        | Qt::ItemNeverHasChildren;
        // Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled
    }
};

#endif // PALETTEM_H
