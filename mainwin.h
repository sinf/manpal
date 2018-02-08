#ifndef MAINWIN_H
#define MAINWIN_H

#include <QMainWindow>
#include <QImage>

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

private slots:
    bool load_src(const QString &);
    void open();

    void scaleSrc();
    void preview();
    void setDitherMethod(int x) { dither_method=x; preview(); }

private:
    Ui::MainWin *ui;
    QImage img_src;
    int dither_method;

    float pal[3][256];
    int pal_c;
};

#endif // MAINWIN_H
