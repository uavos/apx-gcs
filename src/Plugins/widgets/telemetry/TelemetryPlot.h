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
#include <qwt_legend.h>
#include <qwt_legend_label.h>
#include <qwt_math.h>
#include <qwt_picker_machine.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_scaleitem.h>
#include <qwt_plot_zoomer.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_widget.h>
#include <qwt_series_data.h>
#include <qwt_symbol.h>
#include <qwt_text.h>
//=============================================================================
class TelemetryPlot : public QwtPlot
{
    Q_OBJECT
public:
    TelemetryPlot(QWidget *parent = 0);
    ~TelemetryPlot();

    QwtPlotCurve *addCurve(const QString &name,
                           const QString &descr,
                           const QString &units,
                           const QPen &pen);

    void resetData();

    void showCurves(bool on = true, const QStringList &names = QStringList(), bool toggle = false);

    void saveSettings();
    void restoreSettings();

    quint64 timeCursorValue();

    QwtPlotCurve *calc;

    void addEvent(double time, const QString &text, QColor color = QColor());

protected:
    QwtPlotPicker *picker;
    QwtPlotPicker *pickerPoint;
    QwtPlotZoomer *zoomer;
    QwtLegend *legend;
    QwtPlotGrid *grid;
    QwtPlotPanner *panner, *panner2;
    QwtPlotMagnifier *magX, *magY, *mag;
    QwtPlotMarker *timeCursor;

    void mouseReleaseEvent(QMouseEvent *event);

private:
    void refreshCalculated();
    QString expCalc;
    int m_progress;
    void setProgress(int v);

    QTimer cursorReplotTimer;

    QList<QwtPlotMarker *> events;
    bool m_eventsVisible;

private slots:
    void pointSelected(const QPointF &pos);
    void showCurve(const QVariant &itemInfo, bool on, int index = -1);

signals:
    void itemVisibleChanged(QwtPlotItem *item);
    void timeCursorChanged(double v);

    void progressChanged(int v);

public slots:
    void resetZoom();

    void setTimeCursor(quint64 time_ms, bool doReplot = true);

    void copyFromPlot(TelemetryPlot *plot);

    bool eventsVisible() const;
    void setEventsVisible(bool v);
};
//=============================================================================
class PlotPicker : public QwtPlotPicker
{
    Q_OBJECT
public:
    PlotPicker(QWidget *canvas)
        : QwtPlotPicker(QwtPlot::xBottom,
                        QwtPlot::yLeft,
                        QwtPicker::VLineRubberBand,
                        QwtPicker::ActiveOnly,
                        canvas)
    {}

protected:
    QwtText trackerText(const QPoint &pos) const;

private:
    double sampleValue(const QwtPlotCurve *curve, double t) const;
};
//=============================================================================
class PlotMagnifier : public QwtPlotMagnifier
{
    Q_OBJECT
public:
    PlotMagnifier(QWidget *canvas)
        : QwtPlotMagnifier(canvas)
    {
        parentWidget()->setMouseTracking(true);
    }

protected:
    QPoint mwPos;
    void rescale(double factor);
    void widgetMouseMoveEvent(QMouseEvent *mouseEvent);
};
//=============================================================================
class LegendItem : public QwtLegendLabel
{
public:
    LegendItem()
        : QwtLegendLabel()
    {}

protected:
    void paintEvent(QPaintEvent *e);
};
//=============================================================================
class PlotLegend : public QwtLegend
{
public:
    PlotLegend(QWidget *parent = 0);
    QWidget *createWidget(const QwtLegendData &data) const;
};
//=============================================================================
#endif
