#ifndef PALETTEM_H
#define PALETTEM_H
#include <QObject>
#include <QModelIndex>
#include <QAbstractItemModel>
#include <QAbstractTableModel>
#include "vec3.h"

// last color is never used
extern ivec3 the_pal_iv[257]; // linear color space
extern QColor the_pal[257]; // qt color space (nonlinear?)
extern int the_pal_c;

void set_color(int i, QColor c);
int add_color(QColor c);
void del_color(int i);
int map_palette(ivec3);
void sort_palette();

// slow but accurate
float sRGBtoLf(float c);
float LtosRGBf(float c);

extern int sRGBtoL_table[0x8000];
extern int LtosRGB_table[0x8000];
int sRGBtoL(int c);
int LtosRGB(int c);
void make_tables(); // initialize tables used above

class PaletteM : public QAbstractTableModel
{
public:
    QColor *pal; // always 256 entries

    PaletteM(QObject *p=nullptr) : QAbstractTableModel(p) { pal = the_pal; }

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

    QModelIndex getLast();
};

extern const QString color_format_help;
QString format_color(QColor c0, QString column );
QString format_pal(QString s, QString const &end, QString const & color_fmt, QColor const pal[], int n_colors);

#endif // PALETTEM_H
