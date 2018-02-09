#include <cmath>
#include <cassert>
#include <iostream>
#include <QFileDialog>
#include <QStandardPaths>
#include <QImageReader>
#include <QImageWriter>
#include <QPixmap>
#include <QMessageBox>
#include <QScrollBar>
#include "mainwin.h"
#include "ui_mainwin.h"
#include "palettem.h"

#define DITHER_MODES "None","Random","Jarvis Judice Ninke","Sierra 2-row", "Sierra 3-row", "Sierra Lite"
int ed_err_fract = 1024;
int ed_pingpong_enable = 0;

template<typename F>
void adjust_table_cells(F set_sz, int dim, int n)
{
    float wf = dim / (float) n;
    float r = wf; // right edge of current cell
    int l = 0; // left edge of current cell
    for( int i=0; i<n; ++i ) {
        int w = (int) r - l;
        l += w;
        r += wf;
        set_sz(i,w);
    }
}

static void cram_table(QTableView *t)
{
    auto m = t->model();
    int
        nc = m->columnCount(), nr = m->rowCount(),
        w = t->width(), h = t->height();
    adjust_table_cells(
        [&](int a,int b){t->setColumnWidth(a,b);}, w, nc);
    adjust_table_cells(
        [&](int a,int b){t->setRowHeight(a,b);}, h, nr);
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

static float sRGBtoLf(float c) {
    return c > 0.04045f ? powf((c+0.055f)*(1./1.055f),2.4f) : c/12.92f;
}
static float LtosRGBf(float c) {
    return c > .0031308f ? 1.055f*powf(c,1/2.4f) - 0.055 : c*12.92f;
}

static int sRGBtoL_table[0x8000];
static int LtosRGB_table[0x8000];

static int clip_s16(int x)
{
    return x < 0 ? 0 : ( x > 0x7fff ? 0x7fff : x );
}

void make_tables()
{
    const int k = 0x8000;
    const float df = 1.0f / k;
    float f = 0;
    for( int i=0; i<k; ++i ) {
        LtosRGB_table[i] = LtosRGBf( f ) * k;
        sRGBtoL_table[i] = sRGBtoLf( f ) * k;
        assert(LtosRGB_table[i] >= 0 && LtosRGB_table[i] < k);
        assert(sRGBtoL_table[i] >= 0 && sRGBtoL_table[i] < k);
        f += df;
    }
}

static int rnd() // -32767 .. 32767
{
    int c = (unsigned short) rand() & 0xfffeu;
    return 0x7fff - c;
}

static int rndh() // -16383 .. 16383
{
    int c = (unsigned short) rand() & 0x7ffeu;
    return 0x3fff - c;
}

#if 1
static int sRGBtoL(int c) { return sRGBtoL_table[c & 0x7fff]; }
static int LtosRGB(int c) { return LtosRGB_table[c & 0x7fff]; }
#else
static int sRGBtoL(int c) { return sRGBtoLf((1./0x7fff)*c)*0x7fff; }
static int LtosRGB(int c) { return LtosRGBf((1./0x7fff)*c)*0x7fff; }
#endif

static int qr(int x) { return (x + rndh() > 16384)*32767; }

MainWin::MainWin(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWin)
{
    ui->setupUi(this);
    ui->dit_mode->addItems({DITHER_MODES});

    auto m = new PaletteM(this);
    ui->tbpal->setModel(m);
    ui->hsplit3->setSizes({20,80,20});

    load_src("/home/arho/Pictures/t3_5uyryd.jpeg");
    scaleSrc();
}

MainWin::~MainWin()
{
    delete ui;
}

static void initializeImageFileDialog(QFileDialog &dialog, QFileDialog::AcceptMode acceptMode)
{
    static bool firstDialog = true;

    if (firstDialog) {
        firstDialog = false;
        const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
    }

    QStringList mimeTypeFilters;
    const QByteArrayList supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen
        ? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
    foreach (const QByteArray &mimeTypeName, supportedMimeTypes)
        mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/jpeg");
    if (acceptMode == QFileDialog::AcceptSave)
        dialog.setDefaultSuffix("jpg");
}

/*
 * gamma-correct scaling
 * use nearest scaling when upscaling, linear? filter when downscaling
 */
auto gscaled(QImage const &i, int w, int h, Qt::AspectRatioMode m)
{
    auto t = i.width() > w && i.height() > h ?
    Qt::SmoothTransformation : Qt::FastTransformation;
    //w &= ~3; h &= ~3;
    return filter(filter(i,sRGBtoL).scaled(w,h,m,t),LtosRGB);
}

bool MainWin::load_src(const QString &fileName)
{
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    QImage newImage = reader.read();
    if (newImage.isNull()) {
        QMessageBox::information(this,
QGuiApplication::applicationDisplayName(),
tr("Cannot load %1: %2")
.arg(QDir::toNativeSeparators(fileName), reader.errorString()));
        return false;
    }

    int r = 500;
    if (newImage.width() > r || newImage.height() > r) {
        newImage = gscaled(newImage, r, r, Qt::KeepAspectRatio);
        std::cerr << "reduced input image size to "
        << newImage.width() << 'x' << newImage.height() << '\n';
    }

    img_src = newImage.convertToFormat(QImage::Format_RGBX8888);
    return true;
}

static void setImg(QLabel *la, QImage const &im)
{
    int w = la->width(), h = la->height();
    la->setPixmap(QPixmap::fromImage(gscaled(im,w,h,Qt::KeepAspectRatio)));
}

void MainWin::scaleSrc()
{
    if (!img_src.isNull()) {
        setImg(ui->srv_view, img_src);
        preview();
    }

    cram_table( ui->tbpal );
}

void MainWin::open()
{
    QFileDialog dialog(this, tr("Open File"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);
    for(;;) {
        if (dialog.exec() != QDialog::Accepted) break;
        if (load_src(dialog.selectedFiles().first())) break;
    }
    scaleSrc();
}

void MainWin::resizeEvent(QResizeEvent *ev)
{
    (void) ev;
    scaleSrc();
}

auto dither_rnd(QImage const &p)
{
    return filter( p,
    [](int x) {
        return LtosRGB(qr(sRGBtoL(x)));
    });
}

static int pal_lookup(int x)
{
    return (x > (int)(LtosRGBf(0.5f)*32768))*32767;
}

auto quant_clip(QImage const &p)
{
    return filter( p, pal_lookup );
}

template<int const R0[], int const R1[], int const R2[], int cols, int off_x, int shr>
struct DitherED {
    int *buf[4];
    int img_w;
    int cur_x_=0;
    int pingpong=0;
    int pingpong_counter=0;
    int cur_x_inc=1;

    void forward()
    {
        cur_x_inc = 1;
        pingpong = 0;
        cur_x_ = 0;
    }

    void reverse()
    {
        cur_x_inc = -1;
        pingpong = -1;
        cur_x_ = img_w - 1;
    }

    DitherED(int w)
    {
        img_w = w;
        int bufsize = w * sizeof(buf[0][0]);
        for( int i=0; i<4; ++i ) {
            buf[i] = new int[w + 32] + 16;
            memset(buf[i], 0, bufsize);
        }
    }

    ~DitherED()
    {
        for( int i=0; i<4; ++i )
            delete ( buf[i] - 16 );
    }

    void endln() {
        int *b0 = buf[0];
        buf[0] = buf[1];
        buf[1] = buf[2];
        buf[2] = buf[3];
        buf[3] = b0;
    }

    int pixel(int c0, int cur_x, int neg)
    {
        int cur_x1 = cur_x + ( neg & 1 );
        int cur_e = buf[0][cur_x] >> shr;
        int c1 = pal_lookup( c0 - cur_e );
        int e = c1 - c0;
        //c1 = clip_s16(abs(e));

        e = e * ed_err_fract >> 10; // reduce distributed error by some fraction
        buf[3][cur_x] = 0; // wipe the next bottom line
        for( int dx=1; dx<cols-off_x; dx++)
            buf[0][cur_x1 + (dx^neg)] += e * R0[dx-1];
        for( int dx=0; dx<cols; dx++) {
            int xx = cur_x1 + ((dx - off_x) ^ neg );
            if (R1 != nullptr) buf[1][xx] += e * R1[dx];
            if (R2 != nullptr) buf[2][xx] += e * R2[dx];
        }
        return c1;
    }

    int pixel(int c0)
    {
        int c = pixel(c0, cur_x_, pingpong);
        cur_x_ += cur_x_inc;
        if ( (unsigned) cur_x_ >= (unsigned) img_w ) {
            cur_x_ = 0;
            endln();
            if (ed_pingpong_enable && ++pingpong_counter == 15) {
                pingpong_counter = 0;
                if (pingpong) forward(); else reverse();
            }
        }
        return c;
    }
};

#define K(x) (4096/48*x)
static constexpr int JJN0[] =                {K(7),K(5)};
static constexpr int JJN1[] = {K(3),K(5),K(7),K(5),K(3)};
static constexpr int JJN2[] = {K(1),K(3),K(5),K(3),K(1)};
typedef DitherED<JJN0,JJN1,JJN2, 5, 3, 12> DitherJJN;

static constexpr int S2R0[] =       {4,3};
static constexpr int S2R1[] = {1,2,3,2,1};
typedef DitherED<S2R0,S2R1,nullptr, 5, 3, 4> DitherS2;

static constexpr int S3R0[] =       {5,3};
static constexpr int S3R1[] = {2,4,5,4,2};
static constexpr int S3R2[] = {0,2,3,2,0};
typedef DitherED<S3R0,S3R1,nullptr, 5, 3, 5> DitherS3;

static constexpr int SL0[] =    {2};
static constexpr int SL1[] = {1, 1};
typedef DitherED<SL0,SL1,nullptr, 2, 1, 2> DitherSL;

template<typename T>
auto dither_ed(QImage const &p)
{
    int w = p.width();
    T r(w), g(w), b(w);
    return filter2( p,
        [&r] (int c) { return r.pixel(c); },
        [&g] (int c) { return g.pixel(c); },
        [&b] (int c) { return b.pixel(c); },
        [] (int) { return 0; }
    );
}

auto quantize(QImage const &p, int dither_method)
{
    char tmp[][100] = {DITHER_MODES};
    static_assert(sizeof(tmp)/100 == 6, "aa");
    switch(dither_method) {
        case 0: return quant_clip(p);
        case 1: return dither_rnd(p);
        case 2: return dither_ed<DitherJJN>(p);
        case 3: return dither_ed<DitherS2>(p);
        case 4: return dither_ed<DitherS3>(p);
        case 5: return dither_ed<DitherSL>(p);
        default: return p;
    }
}

static auto prefilter(QImage const &p)
{
    return p;
    // gray test
    return p.convertToFormat(QImage::Format_Grayscale8).convertToFormat(QImage::Format_RGBX8888);
}

void MainWin::preview()
{
    if (img_src.isNull()) return;
    int ss = ui->srv_view->width() * ui->srv_view->height();
    int is = img_src.width() * img_src.height();
    if ( ss < is && ui->dit_ss->isChecked() ) {
        // dither in screen space for faster updates
        auto p = ui->srv_view->pixmap()->toImage();
        ui->out_view->setPixmap(QPixmap::fromImage(quantize(prefilter(p), dither_method)));
    } else {
        // dither in image space
        setImg(ui->out_view, quantize(prefilter(img_src), dither_method));
    }
}
