#include <cmath>
#include <cassert>
#include <iostream>
#include <QFileDialog>
#include <QStandardPaths>
#include <QImageReader>
#include <QImageWriter>
#include <QPixmap>
#include <QMessageBox>
#include "mainwin.h"
#include "ui_mainwin.h"

template<typename F> QImage filter(QImage const &src, F f)
{
    QImage dst(src.size(), src.format());
    int y, x, w=src.width(), h=src.height();
    for( y=0; y<h; ++y ) {
        uchar const *s = src.scanLine(y);
        uchar *d = dst.scanLine(y);
        for( x=0; x<w; ++x ) {
            for( int c=0; c<3; ++c ) {
                d[4*x+c] = f( s[4*x+c] << 7 ) >> 7;
            }
        }
    }
    return dst;
}

static float sRGBtoLf(float c) {
    return c > 0.04045f ? powf((c+0.055f)*(1./1.055f),2.4f) : c/12.92f;
}
static float LtosRGBf(float c) {
    return c > .0031308f ? 1.055f*powf(c,1/2.4f) - 0.055 : c*12.92f;
}

static int sRGBtoL_table[0x8000];
static int LtosRGB_table[0x8000];
static void make_tables()
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

static int sRGBtoL(int c) { return sRGBtoL_table[c & 0x7fff]; }
static int LtosRGB(int c) { return LtosRGB_table[c & 0x7fff]; }
static int qr(int x) { return (x + rndh() > 16384)*32767; }

MainWin::MainWin(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWin)
{
    ui->setupUi(this);
    ui->comboBox->addItems({"None","Random"});
    make_tables();
    load_src("/home/arho/Pictures/t3_5uyryd.jpeg");
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
static auto gscaled(QImage const &i, int w, int h, Qt::AspectRatioMode m)
{
    auto t = i.width() > w && i.height() > h ?
    Qt::SmoothTransformation : Qt::FastTransformation;
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
        std::cerr << "reducing input image size to "
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

static auto quantize(QImage const &p, int dither_method)
{
    switch(dither_method) {
        case 1:
            return filter( p,
            [](int x) {
                return LtosRGB(qr(sRGBtoL(x)));
            });
            break;
        default:
        case 0:
            return filter( p,
            [](int x) {
                return (x > (int)(LtosRGBf(0.5f)*32768))*32767;
            });
            break;
    }
}

void MainWin::preview()
{
    if (img_src.isNull()) return;
    int ss = ui->srv_view->width() * ui->srv_view->height();
    int is = img_src.width() * img_src.height();
    if ( ss < is && ui->dit_ss->isChecked() ) {
        // dither in screen space for faster updates
        auto p = ui->srv_view->pixmap()->toImage();
        ui->out_view->setPixmap(QPixmap::fromImage(quantize(p, dither_method)));
    } else {
        // dither in image space
        setImg(ui->out_view, quantize(img_src, dither_method));
    }
}
