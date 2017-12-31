#include <qwt_plot.h>
#include <qwt_plot_spectrogram.h>

class QwtMatrixRasterData;

class Plot: public QwtPlot
{
    Q_OBJECT

public:
    enum ColorMap
    {
        RGBMap,
        IndexMap,
        HueMap,
        AlphaMap
    };

    Plot( QWidget * = NULL );
    void setMetrix(QVector<double> metrix, int width);

public Q_SLOTS:
    void showContour( bool on );
    void showSpectrogram( bool on );
    void setColorMap( int );
    void setAlpha( int );

private:
    QwtPlotSpectrogram     *d_spectrogram;
    QwtMatrixRasterData    *data;
    int                     d_mapType;
    int                     d_alpha;
};
