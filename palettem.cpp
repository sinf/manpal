#include <QBrush>
#include <QColor>
#include "palettem.h"

PaletteM::PaletteM(QObject *p) : QAbstractTableModel(p)
{
    pal_c = 0;
}

int PaletteM::getidx(const QModelIndex &i) const
{
    int n = i.row() * columnCount(i) + i.column();
    return n < 0 || n > 255 ? 0 : n;
}

// return a color that is always visible on specified background color
static QColor textcolor(QColor bg)
{
    int msk = 256;
    int off = 128;
    int h, s, v;
    bg.getHsv(&h, &s, &v);
    h = ( h + off ) & msk;
    v = ( v + off ) & msk;
    return QColor::fromHsv(h,s,v);
}

QVariant PaletteM::data(const QModelIndex &index, int role) const
{
    auto c = pal[getidx(index)];
    switch(role) {
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
            return QVariant(c.name());
        case Qt::BackgroundRole:
            return QVariant(QBrush(c));
        case Qt::ForegroundRole:
        case Qt::DisplayRole: return QVariant("");
        default:
            return QVariant();
    }
}


bool PaletteM::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QColor c(value.toString());
    auto ok = c.isValid();
    if ( ok ) pal[getidx(index)] = c;
    return ok;
}
