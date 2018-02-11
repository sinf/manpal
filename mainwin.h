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
    QString export_s();

public slots:
    bool load_src(const QString &);
    void open();

    // visual feedback image
    void scaleSrc();
    void preview();
    void setDitherMethod(int x) { dither_method=x; preview(); }
    void setDitherPP(int x) { ed_pingpong_enable=x; preview(); }
    void setDitherE(int x);
    void resetDitherE();
    void setColorCount(int x);

    // palette edits
    QColor sample();
    void addColor();
    void delColors();
    void setColor();
    void colorEditMode(bool);
    void refreshTable();
    void sortColors();
    void genGray();
    void genHist();

    // export functions
    void exp_preview();
    void exp_msg();
    void exp_file();
    void exp_preset(int);
    void exp_help();

private:
    Ui::MainWin *ui;
    QImage img_src;
    QColor sampled_color;
    int dither_method;
    bool live_edit_on;

protected:
    void keyPressEvent(QKeyEvent *);
    void mouseMoveEvent(QMouseEvent *);
};

#endif // MAINWIN_H
