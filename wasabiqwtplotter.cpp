#include "wasabiqwtplotter.h"
#include <qlabel.h>
#include <qpainter.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_curve.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_widget.h>
#include <qwt_legend.h>
#include <qwt_legend_item.h>
#include <qwt_plot_canvas.h>
// for CpuStat
#include <qstringlist.h>
#include <qfile.h>
#include <qtextstream.h>

PADStat::PADStat()
{
    //lookUp( procValues );
}

QTime PADStat::upTime() const
{
    QTime t;
    for ( int i = 0; i < NValues; i++ )
        t = t.addSecs( int( procValues[i] / 100 ) );

    return t;
}

void PADStat::statistic( double &p, double &a, double &d )
{
    p = (double)wasabi->getEAfromID()->getPValue();
    a = (double)wasabi->getEAfromID()->getAValue();
    d = (double)wasabi->getEAfromID()->getDValue();
    // TODO: Fill the values from wasabi
}

//void PADStat::lookUp( double values[NValues] ) const
//{
//    {
//        QTextStream textStream( &file );
//        do
//        {
//            QString line = textStream.readLine();
//            line = line.trimmed();
//            if ( line.startsWith( "cpu " ) )
//            {
//                const QStringList valueList =
//                    line.split( " ",  QString::SkipEmptyParts );
//                if ( valueList.count() >= 5 )
//                {
//                    for ( int i = 0; i < NValues; i++ )
//                        values[i] = valueList[i+1].toDouble();
//                }
//                break;
//            }
//        }
//        while( !textStream.atEnd() );
//    }
//}


class TimeScaleDraw: public QwtScaleDraw
{
public:
    TimeScaleDraw( const QTime &base ):
        baseTime( base )
    {
    }
    virtual QwtText label( double v ) const
    {
        QTime upTime = baseTime.addSecs( ( int )v );
        return upTime.toString();
    }
private:
    QTime baseTime;
};

class Background: public QwtPlotItem
{
public:
    Background()
    {
        setZ( 0.0 );
    }

    virtual int rtti() const
    {
        return QwtPlotItem::Rtti_PlotUserItem;
    }

    virtual void draw( QPainter *painter,
        const QwtScaleMap &, const QwtScaleMap &yMap,
        const QRectF &canvasRect ) const
    {
        QColor c( Qt::white );
        QRectF r = canvasRect;

        for ( int i = 100; i > -100; i -= 10 )
        {
            r.setBottom( yMap.transform( i - 10 ) );
            r.setTop( yMap.transform( i ) );
            painter->fillRect( r, c );

            c = c.dark( 105 );
        }
    }
};

class PADCurve: public QwtPlotCurve
{
public:
    PADCurve( const QString &title ):
        QwtPlotCurve( title )
    {
        setRenderHint( QwtPlotItem::RenderAntialiased );
    }

    void setColor( const QColor &color )
    {
        QColor c = color;
        c.setAlpha( 150 );

        setPen( c );
        setBrush( c );
    }
};

PADPlot::PADPlot( QWidget *parent ):
    QwtPlot( parent ),
    dataCount( 0 )
{
    setAutoReplot( false );

    canvas()->setBorderRadius( 10 );

    plotLayout()->setAlignCanvasToScales( true );

    QwtLegend *legend = new QwtLegend;
    legend->setItemMode( QwtLegend::CheckableItem );
    insertLegend( legend, QwtPlot::RightLegend );

    setAxisTitle( QwtPlot::xBottom, " System Uptime [h:m:s]" );
    setAxisScaleDraw( QwtPlot::xBottom,
        new TimeScaleDraw( padStat.upTime() ) );
    setAxisScale( QwtPlot::xBottom, 0, HISTORY );
    setAxisLabelRotation( QwtPlot::xBottom, -50.0 );
    setAxisLabelAlignment( QwtPlot::xBottom, Qt::AlignLeft | Qt::AlignBottom );

    /*
     In situations, when there is a label at the most right position of the
     scale, additional space is needed to display the overlapping part
     of the label would be taken by reducing the width of scale and canvas.
     To avoid this "jumping canvas" effect, we add a permanent margin.
     We don't need to do the same for the left border, because there
     is enough space for the overlapping label below the left scale.
     */

    QwtScaleWidget *scaleWidget = axisWidget( QwtPlot::xBottom );
    const int fmh = QFontMetrics( scaleWidget->font() ).height();
    scaleWidget->setMinBorderDist( 0, fmh / 2 );

    setAxisTitle( QwtPlot::yLeft, "Values" );
    setAxisScale( QwtPlot::yLeft, -100, 100 );

    Background *bg = new Background();
    bg->attach( this );

    //CpuPieMarker *pie = new CpuPieMarker();
    //pie->attach( this );

    PADCurve *curve;

    curve = new PADCurve( "Pleasure" );
    curve->setColor( Qt::red );
    curve->attach( this );
    data[Arousal].curve = curve;

    curve = new PADCurve( "Arousal" );
    curve->setColor( Qt::blue );
    curve->setZ( curve->z() - 1 );
    curve->attach( this );
    data[Pleasure].curve = curve;

    curve = new PADCurve( "Dominance" );
    curve->setColor( Qt::black );
    curve->setZ( curve->z() - 2 );
    curve->attach( this );
    data[Dominance].curve = curve;

    curve = new PADCurve( "Idle" );
    curve->setColor( Qt::darkCyan );
    curve->setZ( curve->z() - 3 );
    curve->attach( this );
    data[Idle].curve = curve;

    showCurve( data[Arousal].curve, true );
    showCurve( data[Pleasure].curve, true );
    showCurve( data[Dominance].curve, false );
    showCurve( data[Idle].curve, false );

    for ( int i = 0; i < HISTORY; i++ )
        timeData[HISTORY - 1 - i] = i;

    ( void )startTimer( 1000 ); // 1 second

    connect( this, SIGNAL( legendChecked( QwtPlotItem *, bool ) ),
        SLOT( showCurve( QwtPlotItem *, bool ) ) );
}

void PADPlot::setWASABI(WASABIEngine* wasabi)
{
    padStat.wasabi = wasabi;
}

void PADPlot::timerEvent( QTimerEvent * )
{
    for ( int i = dataCount; i > 0; i-- )
    {
        for ( int c = 0; c < NCpuData; c++ )
        {
            if ( i < HISTORY )
                data[c].data[i] = data[c].data[i-1];
        }
    }

    padStat.statistic( data[Pleasure].data[0], data[Arousal].data[0], data[Dominance].data[0] );

    //data[Total].data[0] = data[Pleasure].data[0] + data[Arousal].data[0];
    data[Idle].data[0] = 0.0;

    if ( dataCount < HISTORY )
        dataCount++;

    for ( int j = 0; j < HISTORY; j++ )
        timeData[j]++;

    setAxisScale( QwtPlot::xBottom,
        timeData[HISTORY - 1], timeData[0] );

    for ( int c = 0; c < NCpuData; c++ )
    {
        data[c].curve->setRawSamples(
            timeData, data[c].data, dataCount );
    }

    replot();
}

void PADPlot::showCurve( QwtPlotItem *item, bool on )
{
    item->setVisible( on );

    QwtLegendItem *legendItem =
        qobject_cast<QwtLegendItem *>( legend()->find( item ) );

    if ( legendItem )
        legendItem->setChecked( on );

    replot();
}

WASABIqwtPlotter::WASABIqwtPlotter(QWidget *parent, WASABIEngine *wasabi) :
    QMainWindow(parent)
{
   // this->wasabi = wasabi;
    vBox = new QWidget(this);
    vBox->setWindowTitle( "Cpu Plot" );

    plot = new PADPlot( vBox );
    plot->setTitle( "History" );

    const int margin = 5;
    plot->setContentsMargins( margin, margin, margin, margin );
    plot->setWASABI(wasabi);

    QString info( "Press the legend to en/disable a curve" );

    QLabel *label = new QLabel( info, this );

    layout = new QVBoxLayout( vBox );
    layout->addWidget( plot );
    layout->addWidget( label );

    vBox->resize( 600, 400 );
    vBox->show();
}

QSize WASABIqwtPlotter::minimumSizeHint() const
{
    return QSize(600, 400);
}

QSize WASABIqwtPlotter::sizeHint() const
{
    return QSize(600, 400);
}
