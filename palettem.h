#ifndef PALETTEM_H
#define PALETTEM_H
#include <QObject>
#include <QModelIndex>
#include <QAbstractItemModel>
#include <QAbstractTableModel>
#include "vec3.h"

extern QColor the_pal[256];
extern int the_pal_c;
extern int sRGBtoL_table[0x8000];
extern int LtosRGB_table[0x8000];

QColor linearize(QColor c);
QColor unlinearize(QColor c);
int map_palette(ivec3);
ivec3 unmap_palette(int i);
float sRGBtoLf(float c);
float LtosRGBf(float c);
void make_tables();
int sRGBtoL(int c);
int LtosRGB(int c);

#define linz3(v) ((v)&0x7fff).lookup(sRGBtoL_table)
#define unlinz3(v) ((v)&0x7fff).lookup(LtosRGB_table)

class PaletteM : public QAbstractTableModel
{
public:
    QColor *pal; // always expect 256 entries

    PaletteM(QObject *p=nullptr) : QAbstractTableModel(p)
    {
        pal = the_pal;
    }

    int columnCount(const QModelIndex &) const { return 8; }
    int rowCount(const QModelIndex &) const { return 32; }

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
