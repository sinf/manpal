#include <limits.h>
#include <QBrush>
#include <QColor>
#include "palettem.h"

QColor the_pal[256];
int the_pal_c = 0;

float sRGBtoLf(float c) {
    return c > 0.04045f ? powf((c+0.055f)*(1./1.055f),2.4f) : c/12.92f;
}
float LtosRGBf(float c) {
    return c > .0031308f ? 1.055f*powf(c,1/2.4f) - 0.055 : c*12.92f;
}

#if 1
int sRGBtoL(int c) { return sRGBtoL_table[c & 0x7fff]; }
int LtosRGB(int c) { return LtosRGB_table[c & 0x7fff]; }
#else
int sRGBtoL(int c) { return sRGBtoLf((1./0x7fff)*c)*0x7fff; }
int LtosRGB(int c) { return LtosRGBf((1./0x7fff)*c)*0x7fff; }
#endif


void make_tables()
{
    const int k = 0x8000;
    const float df = 1.0f / k;
    float f = 0;
    for( int i=0; i<k; ++i ) {
        LtosRGB_table[i] = LtosRGBf( f ) * k;
        sRGBtoL_table[i] = sRGBtoLf( f ) * k;
        f += df;
    }
}


QColor linearize(QColor c)
{
    return QColor::fromRgbF(
    sRGBtoLf(c.redF()),
    sRGBtoLf(c.greenF()),
    sRGBtoLf(c.blueF()));
}

QColor unlinearize(QColor c)
{
    return QColor::fromRgbF(
    LtosRGBf(c.redF()),
    LtosRGBf(c.greenF()),
    LtosRGBf(c.blueF()));
}

int map_palette(ivec3 ref)
{
    long R=INT_MAX;
    int Ri=0;
    for(int i=0; i<the_pal_c; ++i) {
        long r = (unmap_palette(i) - ref).lensq();
        if ( r < R ) {
            Ri = i;
            R = r;
        }
    }
    return Ri;
}

ivec3 unmap_palette(int i)
{
    return linz3(ivec3(the_pal[i])<<7);
}

int PaletteM::getidx(const QModelIndex &i) const
{
    int n = i.row() * columnCount(i) + i.column();
    return n < 0 || n > 255 ? 0 : n;
}

// return a color that is always visible on specified background color
static QColor textcolor(QColor bg)
{
    QColor c;
    c.setRed(bg.red() ^ 0xff);
    c.setGreen(bg.green() ^ 0xff);
    c.setBlue(bg.blue() ^ 0xff);
    return c;
}

QVariant PaletteM::data(const QModelIndex &index, int role) const
{
    int i = getidx(index);
    auto c = pal[i];
    bool xx = i >= the_pal_c;
    QBrush br;
    switch(role) {
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
            return QVariant(c.name());
        case Qt::BackgroundRole:
            br = QBrush(c);
            if (xx) br.setStyle(Qt::Dense3Pattern);
            return QVariant(br);
        case Qt::ForegroundRole:
            return QVariant(QBrush(textcolor(c)));
        case Qt::DisplayRole:
            return QVariant("");
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
