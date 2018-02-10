#ifndef IMGFILTER_H
#define IMGFILTER_H
#include <QImage>

template<typename F>
QImage filter_rgb(QImage const &src, F f)
{
    QImage dst(src.size(), src.format());
    int y, x, w=src.width(), h=src.height();
    for( y=0; y<h; ++y ) {
        auto s = (int32_t const*) src.scanLine(y);
        auto d = (int32_t*) dst.scanLine(y);
        for( x=0; x<w; x++ ) {
            int32_t rgb = s[x];
            int b = ( rgb & 0xFF ) << 7;
            int g = ( rgb & 0xff00 ) >> 1;
            int r = ( rgb & 0xff0000 ) >> 9;
            d[x] = f( r, g, b );
        }
    }
    return dst;
}

template<typename FR, typename FG, typename FB, typename FA>
QImage filter2(QImage const &src, FR fr, FG fg, FB fb, FA fa)
{
    QImage dst(src.size(), src.format());
    int y, x, w=src.width()*4, h=src.height();
    for( y=0; y<h; ++y ) {
        uchar const *s = src.scanLine(y);
        uchar *d = dst.scanLine(y);
        for( x=0; x<w; x+=4 ) {
            d[x] = fr( s[x] << 7 ) >> 7;
            d[x+1] = fg( s[x+1] << 7 ) >> 7;
            d[x+2] = fb( s[x+2] << 7 ) >> 7;
            d[x+3] = fa( s[x+3] << 7 ) >> 7;
        }
    }
    return dst;
}

template<typename F> QImage filter(QImage const &src, F f)
{
    return filter2<F>(src,f,f,f,f);
}

#endif // IMGFILTER_H
