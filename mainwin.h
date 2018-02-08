#ifndef MAINWIN_H
#define MAINWIN_H

#include <QMainWindow>
#include <QImage>
#include <QTableWidgetItem>

void make_tables();

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
};

#endif // MAINWIN_H
