#include <cmath>
#include <cassert>
#include <iostream>
#include <QFileDialog>
#include <QStandardPaths>
#include <QKeyEvent>
#include <QImageReader>
#include <QImageWriter>
#include <QPixmap>
#include <QMessageBox>
#include <QScrollBar>
#include "mainwin.h"
#include "ui_mainwin.h"
#include "palettem.h"
#include "vec3.h"
#include "dithered.h"
#include "imgfilter.h"
#include "dkm.hpp"

int pack(ivec3 v) {
    int b = 8, m = 255;
    v = v >> 7 & m;
    return v.s[2] | v.s[1] << b | v.s[0] << 2*b;
}

ivec3 unpack(int c) {
    int b = 8, m = 255;
    return ivec3( c >> 2*b, c >> b, c ) & m << 7;
}

// quantize a color
static ivec3 qn3(ivec3 x)
{
    return the_pal_iv[map_palette(x)];
}

template<typename T>
auto dither_ed(QImage const &p)
{
    T ed(p.width());
    return filter_rgb( p,
        [&ed] (int r, int g, int b)
        {
            auto c0 = ivec3(r,g,b).lookup(sRGBtoL_table);
            auto c1 = ed.pixel(c0,qn3);
            auto c2 = (c1 & 0x7fff).lookup(LtosRGB_table);
            return pack(c2);
        }
    );
}

auto simple_q(QImage const &p)
{
    return filter_rgb( p, [](int r, int g, int b) {
        auto x0 = ivec3(r,g,b).lookup(sRGBtoL_table);
        auto x1 = qn3(x0).lookup(LtosRGB_table);
        return pack(x1);
    });
}


static const QStringList qfun_names({
"None",
"Floyd-Steinberg",
"Jarvis Judice Ninke",
"Sierra 3-row",
"Sierra 2-row",
// "Sierra Lite",
});

typedef QImage (*QuantizerFunc)(QImage const&);
static const QuantizerFunc qfun[] = {
simple_q,
dither_ed<DitherFS>,
dither_ed<DitherJJN>,
dither_ed<DitherS3>,
dither_ed<DitherS2>,
// dither_ed<DitherSL>,
};

static const struct {
    QString header, footer, fmt;
} fmt_preset[] = {
    {"", "", "#$(rx)$(gx)$(bx)"},
    {"colors=[", "]", "($(rf), $(gf), $(bf)]"},
};

static const QStringList fmt_preset_names(
{
"HTML hex",
"Python float"
});

template<typename F>
void adjust_table_cells(F set_sz, int dim, int n)
{
    float wf = dim / (float) n;
    float r = wf; // right edge of current cell
    int l = 0; // left edge of current cell
    for( int i=0; i<n; ++i ) {
        int w = (int) ( r + 0.5f ) - l;
        l += w;
        r += wf;
        set_sz(i,w);
    }
}

static void cram_table(QTableView *t, int nc, int nr)
{
    int w = t->width(), h = t->height();
    nc = std::max(nc, 8);
    nr = std::max(nr, 1);
    adjust_table_cells(
        [&](int a,int b){t->setColumnWidth(a,b);}, w, nc);
    adjust_table_cells(
        [&](int a,int b){t->setRowHeight(a,b);}, h, nr);
}

MainWin::MainWin(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWin)
{
    ui->setupUi(this);
    ui->dit_mode->addItems(qfun_names);
    ui->exp_preset->addItems(fmt_preset_names);
    ui->tbpal->setModel(new PaletteM(this));
    ui->hsplit3->setSizes({20,80,20});
    load_src("img/uyryd.jpg");
    scaleSrc();
}

MainWin::~MainWin()
{
    delete ui;
}

void MainWin::refreshTable()
{
    auto t = ui->tbpal;
    auto m = t->model();
    ui->pal_c->setValue(the_pal_c); // update color count spinner
    cram_table(t, m->columnCount(), dpyRows()); // rescale table
    m->dataChanged(m->index(1,1),m->index(m->columnCount(),m->rowCount())); // repaint the color table
}

void MainWin::setColorCount(int x)
{
    the_pal_c = x < 0 ? 0 : ( x > 256 ? 256 : x );
    refreshTable();
    preview(); // palette changed, thus preview image also changed
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
    auto supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen
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
auto gscaled(QImage const &i, int w, int h, Qt::AspectRatioMode m, int smooth=1)
{
    auto t = smooth ? //i.width() > w || i.height() > h ?
    Qt::SmoothTransformation : Qt::FastTransformation;
    //w &= ~3; h &= ~3;
    return filter(filter(i,sRGBtoL).scaled(w,h,m,t),LtosRGB);
}

bool MainWin::load_src(const QString &fileName)
{
    QImageReader reader(fileName);
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    // auto-rotate jpegs if metadata says so
    reader.setAutoTransform(true);
#endif
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

    refreshTable();
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

static QImage quantizeImg(QImage const &p, int mode)
{
    return qfun[mode](p);
}

void MainWin::preview()
{
    if (img_src.isNull()) return;
    //int ss = ui->srv_view->width() * ui->srv_view->height();
    //int is = img_src.width() * img_src.height();
    if ( ui->dit_ss->isChecked() ) {
        // dither in screen space
        auto p = ui->srv_view->pixmap()->toImage();
        ui->out_view->setPixmap(QPixmap::fromImage(quantizeImg(p, dither_method)));
    } else {
        // dither in image space
        setImg(ui->out_view, quantizeImg(img_src, dither_method));
    }
}

QColor MainWin::sample()
{
    auto a = window()->cursor().pos();
    auto g = this->grab(QRect(this->mapFromGlobal(a),QSize(1,1)));
    auto x = g.toImage().pixel(0,0);
    auto c = QColor(x);
    ui->color_box->setStyleSheet(QString("background-color: %1;").arg(c.name()));
    sampled_color = c;
    return c;
}

void MainWin::setColor()
{
    auto ii=ui->tbpal->currentIndex();
    auto index=ii.column()+ii.row()*8;
    if (index >= 0 && index < the_pal_c) {
        set_color(index,sample());
        refreshTable();
        preview();
    }
}

void MainWin::keyPressEvent(QKeyEvent *e)
{
    switch(e->key()) {
        case Qt::Key::Key_A:
            sample();
            addColor();
            break;
        case Qt::Key::Key_F:
            sample();
            setColor();
            break;
        default:
            break;
    }
}

void MainWin::mouseMoveEvent(QMouseEvent *e)
{
#if 0
    static int x0=-100, y0=-100;
    int x = e->x(), y = e->y();
    int dx = x-x0;
    int dy = y-y0;
    if ( dx*dx + dy*dy > 7 ) x0=x, y0=y,
#endif
    sample();
    if (live_edit_on){
        setColor();
    }
}

void MainWin::addColor()
{
    if ( the_pal_c < 256 ) {
        auto sel = ui->tbpal->selectionModel();
        auto mo = (PaletteM*) ui->tbpal->model();
        auto last = mo->getLast();
        sel->clearSelection();
        sel->select(last, QItemSelectionModel::Select);
        add_color(sampled_color);
        refreshTable();
    }
}

void MainWin::delColors()
{
    PaletteM *m = static_cast<PaletteM*>(ui->tbpal->model());
    auto sel = ui->tbpal->selectionModel();
    for( auto i : sel->selectedIndexes() ) {
        int j = m->getidx(i);
        if (j < the_pal_c) del_color(j);
    }
    sel->clearSelection();
    refreshTable();
    preview();
}

void MainWin::colorEditMode(bool on)
{
    if (live_edit_on = on) {
        setColor();
    }
}

void MainWin::setDitherE(int x)
{
    ed_err_fract=x;
    ui->dit_ed_fract2->setValue(x);
    preview();
}

void MainWin::resetDitherE()
{
    ui->dit_ed_fract2->setValue(1024);
    setDitherE(1024);
}

void MainWin::sortColors()
{
    sort_palette();
}

#define tdoc(xxx) ui->xxx->document()

QString MainWin::export_s()
{
    return format_pal(
        tdoc(exp_header)->toPlainText(),
        tdoc(exp_footer)->toPlainText(),
        tdoc(exp_fmt)->toPlainText(), the_pal, the_pal_c);
}

void MainWin::exp_preview()
{
    auto s = format_color(the_pal[0], tdoc(exp_fmt)->toPlainText());
    ui->exp_preview->setText(s);
}

void MainWin::exp_msg()
{
    QMessageBox::information(this,
    QGuiApplication::applicationDisplayName(),
    export_s(), tr("Close"));
}

void MainWin::exp_file()
{
    exp_msg();
}

void MainWin::exp_preset(int p)
{
    tdoc(exp_header)->setPlainText(fmt_preset[p].header);
    tdoc(exp_footer)->setPlainText(fmt_preset[p].footer);
    tdoc(exp_fmt)->setPlainText(fmt_preset[p].fmt);
    exp_preview();
}

void MainWin::exp_help()
{
    QMessageBox::information(this,
    QGuiApplication::applicationDisplayName(),
    color_format_help,
    tr("Close"));
}

void MainWin::genGray()
{
    float v = 0;
    float dv = 1.0f / (the_pal_c-1);
    for(int i=0; i<the_pal_c; ++i, v+=dv)
        set_color(i, QColor::fromRgbF(v,v,v));
    refreshTable();
    preview();
}

static auto get_kmeans_data(QImage const &src)
{
    std::vector<std::array<float,3>> data;
    int y, x, w=src.width(), h=src.height();
    for( y=0; y<h; ++y ) {
        auto s = (int32_t const*) src.scanLine(y);
        for( x=0; x<w; x++ ) {
            int32_t rgb = s[x];
            float
            b = ( rgb & 0xFF ),
            g = ( rgb >> 8 & 0xff ),
            r = ( rgb >> 16 & 0xff );
            data.push_back({{r,g,b}});
        }
    }
    return data;
}

void MainWin::genHist()
{
    const int n = the_pal_c;
    auto data = get_kmeans_data(gscaled(img_src,128,80,Qt::IgnoreAspectRatio,1));
    auto mc = dkm::kmeans_lloyd(data, n);

    for( int i=0; i<n; ++i ) {
        auto m = std::get<0>(mc)[i];
        int r = (int) m[2], g = (int) m[1], b = (int) m[0];
        set_color(i, QColor(r,g,b));
    }

    refreshTable();
    preview();
}
