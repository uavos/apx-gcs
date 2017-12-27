/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef TelemetryPlot_H
#define TelemetryPlot_H
//-----------------------------------------------------------------------------
#include <QtWidgets>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_canvas.h>
#include <qwt_legend.h>
#include <qwt_legend_label.h>
#include <qwt_math.h>
#include <qwt_scale_engine.h>
#include <qwt_symbol.h>
#include <qwt_text.h>
#include <qwt_plot_scaleitem.h>
#include <qwt_scale_widget.h>
#include <qwt_series_data.h>
#include <qwt_picker_machine.h>
//=============================================================================
typedef struct {
    QwtPlotCurve *curve;
    QVector<QPointF> points;
} _telemetry_field;
//=============================================================================
class TelemetryPlot: public QwtPlot
{
    Q_OBJECT
public:
    TelemetryPlot(QWidget *parent = 0);
    ~TelemetryPlot();

    QHash<QString,_telemetry_field*> fields; //by name
    QVector<double> times;

    _telemetry_field *fcalculated;

    quint64 timeCursorValue();

protected:
    QwtPlotPicker *picker;
    QwtPlotPicker *pickerPoint;
    QwtPlotZoomer *zoomer;
    QwtLegend *legend;
    QwtPlotGrid *grid;
    QwtPlotPanner *panner,*panner2;
    QwtPlotMagnifier *magX,*magY,*mag;
    QwtPlotMarker *timeCursor;


private slots:
    void pointSelected( const QPointF &pos );

private:
    bool isCopy;

    _telemetry_field * registerField(const QString &varName,const QString &dsc,const QPen &pen);

    void refreshCalculated(void);
    QString expCalc;

    int m_progress;
    void setProgress(int v);

signals:
  void progressChanged(int v);
  void timeCursorChanged(double v);


public slots:
    void resetZoom();

    void setTimeCursor(quint64 time_ms);

    void copyFromPlot(TelemetryPlot *plot);
    void showCurves(bool on=true,const QStringList &names=QStringList(),bool toggle=false);
    void showCurve(const QVariant &itemInfo, bool on, int index = -1);
};
//=============================================================================
class PlotPicker: public QwtPlotPicker
{
    Q_OBJECT
public:
    PlotPicker(QWidget *canvas):QwtPlotPicker(QwtPlot::xBottom,QwtPlot::yLeft,QwtPicker::VLineRubberBand,QwtPicker::ActiveOnly,canvas){}
protected:
    QwtText trackerText(const QPoint &pos)const;
};
//=============================================================================
class PlotMagnifier: public QwtPlotMagnifier
{
    Q_OBJECT
public:
    PlotMagnifier(QWidget *canvas):QwtPlotMagnifier(canvas){parentWidget()->setMouseTracking(true);}
protected:
    QPoint mwPos;
    void rescale(double factor);
    void widgetMouseMoveEvent(QMouseEvent *mouseEvent);
};
//=============================================================================
class LegendItem: public QwtLegendLabel
{
public:
    LegendItem():QwtLegendLabel(){}
protected:
    void paintEvent(QPaintEvent *e);
};
//=============================================================================
class PlotLegend : public QwtLegend
{
public:
    PlotLegend(QWidget *parent = 0);
    QWidget* createWidget(const QwtLegendData &data) const;
};
//=============================================================================
#endif
