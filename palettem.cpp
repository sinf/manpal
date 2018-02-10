#include <cmath>
#include <limits>
#include <cstdlib>
#include <vector>
#include <QBrush>
#include <QColor>
#include "palettem.h"
#include "vec3.h"

int sRGBtoL_table[0x8000];
int LtosRGB_table[0x8000];

QColor the_pal[257];
ivec3 the_pal_iv[257];
int the_pal_c = 0;

void set_color(int i, QColor c)
{
    the_pal[i] = c;
    the_pal_iv[i] = qcolor_to_ivec3_s(c);
}

int add_color(QColor c)
{
    int i = -1;
    if (the_pal_c < 256) set_color( i=the_pal_c++, c );
    return i;
}

void del_color(int x0)
{
    the_pal_c--;
    while ( x0 < 255 ) {
        int x = x0 + 1;
        the_pal[x0] = the_pal[x];
        the_pal_iv[x0] = the_pal_iv[x0];
        x0 = x;
    }
}

int map_palette(ivec3 ref)
{
    long R=LONG_MAX;
    int Ri=0;
    for(int i=0; i<the_pal_c; ++i) {
        long r = (the_pal_iv[i] - ref).lensq<long>();
        if ( r < R ) {
            Ri = i;
            R = r;
        }
    }
    return Ri;
}

static int ccmp(const void* A, const void*B)
{
    auto a = (const QColor *) A;
    auto b = (const QColor *) B;
    int h0 = a->hue(), h1 = b->hue();
    int v0 = a->lightness(), v1 = b->lightness();
    if (h0 < h1) return -1;
    if (h0 > h1) return 1;
    if (v0 < v1) return -1;
    if (v0 > v1) return 1;
    return 0;
}

void sort_palette()
{
    qsort(the_pal, the_pal_c, sizeof(the_pal[0]), ccmp);
    for( int i=0; i<the_pal_c; ++i )
        the_pal_iv[i] = qcolor_to_ivec3_s(the_pal[i]);
}

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

int PaletteM::getidx(const QModelIndex &i) const
{
    int n = i.row() * columnCount(i) + i.column();
    return ( n < 0 || n > 255 ) ? 256 : n;
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
        case Qt::DecorationRole:
            if (!xx)
                return QVariant(c);
        default:
            return QVariant();
    }
}


bool PaletteM::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QColor c(value.toString());
    auto ok = c.isValid();
    if ( ok ) set_color( getidx(index), c );
    return ok;
}

QModelIndex PaletteM::getLast()
{
    int w = 8;
    int r = the_pal_c / w;
    int c = the_pal_c % w;
    return index(r,c);
}

