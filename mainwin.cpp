#include <cmath>
#include <iostream>
#include <QFileDialog>
#include <QStandardPaths>
#include <QImageReader>
#include <QImageWriter>
#include <QPixmap>
#include <QMessageBox>
#include "mainwin.h"
#include "ui_mainwin.h"

template<typename F> auto filter(QImage const &src, F f)
{
    auto dst = new QImage(src.size(), src.format());
    int y, x, w=src.width(), h=src.height();
    for( y=0; y<h; ++y ) {
        uchar const *s = src.scanLine(y);
        uchar *d = dst->scanLine(y);
        for( x=0; x<w; ++x )
            for( int c=0; c<3; ++c )
                d[4*x+c] = f( s[4*x+c] * (1.f/255) ) * 255.f;
    }
    return dst;
}

static float sRGBtoL(float c) {
    return c > 0.04045f ? powf((c+0.055f)*(1./1.055f),2.4f) : c/12.92f;
}
static float LtosRGB(float c) {
    return c > .0031308f ? 1.055f*powf(c,1/2.4f) - 0.055 : c*12.92f;
}
static float rndf() { return (short) rand() * (1./0xffff); }
static float q0(float x) { return x > .5f; }
static float qr(float x) { return x + rndf() > .5f; }

MainWin::MainWin(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWin)
{
    ui->setupUi(this);
    ui->comboBox->addItems({"None","Random"});
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
    return filter(filter(i,sRGBtoL)->scaled(w,h,m,t),LtosRGB);
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
        newImage = *gscaled(newImage, r, r, Qt::KeepAspectRatio);
        std::cerr << "reducing input image size\n";
    }

    img_src = newImage.convertToFormat(QImage::Format_RGBX8888);
    return true;
}

static void setImg(QLabel *la, QImage const &im)
{
    int w = la->width(), h = la->height();
    la->setPixmap(QPixmap::fromImage(*gscaled(im,w,h,Qt::KeepAspectRatio)));
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
            [](float x) {
                return LtosRGB(qr(sRGBtoL(x)));
            });
            break;
        default:
        case 0:
            return filter( p,
            [](float x) {
                return x > LtosRGB(0.5f);
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
        ui->out_view->setPixmap(QPixmap::fromImage(*quantize(p, dither_method)));
    } else {
        // dither in image space
        setImg(ui->out_view, *quantize(img_src, dither_method));
    }
}
