/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

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
#include <qwt_scale_map.h>
#include <qwt_scale_widget.h>
#include <qwt_series_data.h>
#include <qwt_symbol.h>
#include <qwt_text.h>
#include <qwt_widget_overlay.h>

class StatsOverlay;

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
    void resetLegend();

    void showCurves(bool on = true, const QStringList &names = QStringList(), bool toggle = false);

    void saveSettings();
    void restoreSettings();

    quint64 timeCursorValue();

    void push_calc_curve(QwtPlotCurve *calc_curve) { calc_curves.insert(calc_curve, ""); };

    QMap<QwtPlotCurve *, QString> &get_calc_curves() { return calc_curves; };

    void addEvent(double time, const QString &text, QColor color = QColor());

protected:
    QwtPlotPicker *picker;
    QwtPlotPicker *pickerPoint;
    QwtPlotPicker *pickerRange;
    QwtPlotZoomer *zoomer;
    QwtLegend *legend;
    QwtPlotGrid *grid;
    QwtPlotPanner *panner, *panner2;
    QwtPlotMagnifier *magX, *magY, *mag;
    QwtPlotMarker *timeCursor;
    StatsOverlay *statsOverlay;

    void mouseReleaseEvent(QMouseEvent *event);

private:
    QMap<QwtPlotCurve *, QString> calc_curves;
    void refreshCalculated(QwtPlotCurve *curve_calc);

    int m_progress;
    void setProgress(int v);

    QTimer cursorReplotTimer;

    QList<QwtPlotMarker *> events;
    bool m_eventsVisible;

    bool m_statsVisible;
    QwtInterval m_range; // selected time range, invalid when not selected
    double m_rangeStart; // time where selection dragging started
    void updateStats();

    // data of the shown stats, to skip refresh when nothing changed (e.g. cursor moved)
    struct StatsSource
    {
        const QwtPointSeriesData *series;
        size_t size;
        double min; // all values of the curve, for formatting
        double max;

        bool operator==(const StatsSource &other) const // same data
        {
            return series == other.series && size == other.size;
        }
    };
    QwtInterval m_statsRange;
    QList<StatsSource> m_statsSources;

private slots:
    void pointSelected(const QPointF &pos);
    void rangeStarted(const QPointF &pos);
    void rangeMoved(const QPointF &pos);
    void showCurve(const QVariant &itemInfo, bool on, int index = -1);

signals:
    void itemVisibleChanged(QwtPlotItem *item);
    void timeCursorChanged(double v);
    void statsVisibleChanged(bool v);

    void progressChanged(int v);

public slots:
    void replot();

    void resetZoom();

    void setTimeCursor(quint64 time_ms, bool doReplot = true);

    void copyFromPlot(TelemetryPlot *plot);

    bool eventsVisible() const;
    void setEventsVisible(bool v);

    bool statsVisible() const;
    void setStatsVisible(bool v);
};

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

// selected range and stats table drawn over canvas to avoid replots while dragging
class StatsOverlay : public QwtWidgetOverlay
{
    Q_OBJECT
public:
    struct Row
    {
        QString name;
        QColor color;
        QStringList values;
    };

    explicit StatsOverlay(TelemetryPlot *plot);

    void setStats(const QwtInterval &selection, const QString &title, const QList<Row> &rows);

    const QwtInterval &selection() const { return m_selection; }
    const QString &title() const { return m_title; }
    const QList<Row> &rows() const { return m_rows; }

    QRect tableRect() const;

protected:
    void drawOverlay(QPainter *painter) const;

private:
    TelemetryPlot *m_plot;
    QwtInterval m_selection;
    QString m_title;
    QList<Row> m_rows;

    QStringList m_header;
    int m_nameWidth{0};
    int m_valueWidth{0};
    QSize m_tableSize;
};

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

class LegendItem : public QwtLegendLabel
{
public:
    LegendItem()
        : QwtLegendLabel()
    {}

protected:
    void paintEvent(QPaintEvent *e);
};

class PlotLegend : public QwtLegend
{
    Q_OBJECT
public:
    PlotLegend(QWidget *parent = 0);
    QWidget *createWidget(const QwtLegendData &data) const;
    void clearLegenedLabels();

private:
    QList<QPointer<LegendItem>> legendLabels;
    QLineEdit *filter_le = nullptr;

private slots:
    void onFilter(QString);
};
