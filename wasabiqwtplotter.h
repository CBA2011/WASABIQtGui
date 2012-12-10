#ifndef WASABIQWTPLOTTER_H
#define WASABIQWTPLOTTER_H

#include <QMainWindow>
#include <qwt_plot.h>

class WASABIqwtPlotter : public QMainWindow
{
    Q_OBJECT
public:
    explicit WASABIqwtPlotter(QWidget *parent = 0);

    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    
signals:
    
public slots:

private:
    QwtPlot* plot;
    
};

#endif // WASABIQWTPLOTTER_H
