#ifndef WASABIQWTPLOTTER_H
#define WASABIQWTPLOTTER_H

#include <QMainWindow>
#include <qwt_plot.h>
#include <qdatetime.h>
#include <qlayout.h>
#include "WASABIEngine.h"
#include "wasabiqtwindow.h"

#define HISTORY 300 // seconds

class QwtPlotCurve;

class PADStat
{
public:
    PADStat();
    void statistic( double &p, double &a, double &d );
    QTime upTime() const;

    enum Value
    {
        User,
        Nice,
        System,
        Idle,

        NValues
    };
    WASABIEngine* wasabi;
private:
    //void lookUp( double[NValues] ) const;
    double procValues[NValues];
};

class PADPlot : public QwtPlot
{
    Q_OBJECT
public:
    enum CpuData
    {
        Pleasure,
        Arousal,
        Dominance,

        NCpuData
    };

    PADPlot( QWidget * parent = 0);
    const QwtPlotCurve *cpuCurve( int id ) const
    {
        return data[id].curve;
    }
    void setWASABI(WASABIEngine* w);

protected:
    void timerEvent( QTimerEvent *e );

private Q_SLOTS:
    void showCurve( QwtPlotItem *, bool on );

private:
    struct
    {
        QwtPlotCurve *curve;
        double data[HISTORY];
    } data[NCpuData];

    double timeData[HISTORY];

    int dataCount;
    PADStat padStat;
};

class WASABIqwtPlotter : public QMainWindow
{
    Q_OBJECT
public:
    explicit WASABIqwtPlotter(QWidget *parent = 0, WASABIEngine* wasabi = 0);

    QSize minimumSizeHint() const;
    QSize sizeHint() const;
//    WASABIEngine* wasabi;
    
signals:
    
public slots:

private:
    PADPlot* plot;
    QVBoxLayout *layout;
    QWidget* vBox;
    void hideEvent(QHideEvent *event);
    WASABIQtWindow *wasabiWindow;

};

#endif // WASABIQWTPLOTTER_H
