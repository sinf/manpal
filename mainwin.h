#ifndef MAINWIN_H
#define MAINWIN_H

#include <QMainWindow>
#include <QImage>
#include <QTableWidgetItem>

extern int the_pal_c;

extern int
ed_err_fract,// 10 fractional bits. used to limit dither error distribution
ed_pingpong_enable;// alternate diffusion direction each scanline

namespace Ui {
class MainWin;
}

class MainWin : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWin(QWidget *parent = 0);
    ~MainWin();

protected:
    void resizeEvent(QResizeEvent *);
    static int dpyRows() { return the_pal_c+7>>3; }
    QColor sample();

private slots:
    bool load_src(const QString &);
    void open();

    void scaleSrc();
    void preview();
    void setDitherMethod(int x) { dither_method=x; preview(); }
    void setDitherE(int x) { ed_err_fract=x; preview(); }
    void setDitherPP(int x) { ed_pingpong_enable=x; preview(); }
    void setColorCount(int x);

private:
    Ui::MainWin *ui;
    QImage img_src;
    int dither_method;

protected:
    void keyPressEvent(QKeyEvent *);
    void mouseMoveEvent(QMouseEvent *);
};

#endif // MAINWIN_H
