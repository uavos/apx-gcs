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
#include "TelemetryPlot.h"
#include <QJSEngine>
#include <QtGui>

static QString timeText(double t)
{
    return QTime(0, 0).addMSecs(qRound(std::max(t, 0.0) * 1000.0)).toString("hh:mm:ss.zzz");
}

TelemetryPlot::TelemetryPlot(QWidget *parent)
    : QwtPlot(parent)
    , m_progress(0)
    , m_eventsVisible(false)
    , m_statsVisible(false)
    , m_rangeStart(0)
{
    setAutoFillBackground(true);
    setAutoReplot(false);
    setCanvasBackground(QColor(Qt::black));
    setStyleSheet("background-color: rgb(0, 0, 0); gridline-color: rgb(255, 255, 255); color: "
                  "rgb(255, 255, 255);");

    // legend
    legend = new PlotLegend;
    legend->setFrameStyle(QFrame::NoFrame); //|QFrame::Plain);
    legend->setDefaultItemMode(QwtLegendData::Checkable);
    insertLegend(legend, QwtPlot::RightLegend);
    connect(legend,
            SIGNAL(checked(QVariant, bool, int)),
            this,
            SLOT(showCurve(QVariant, bool, int)));

    // grid
    grid = new QwtPlotGrid;
    grid->setMajorPen(QPen(QColor(80, 80, 80), 0, Qt::DotLine));
    grid->attach(this);

    //move pan
    panner = new QwtPlotPanner(canvas());
    panner->setMouseButton(Qt::MiddleButton);
    panner2 = new QwtPlotPanner(canvas());
    panner2->setMouseButton(Qt::LeftButton, Qt::ShiftModifier);

    //zoom
    zoomer = new QwtPlotZoomer(QwtPlot::xBottom, QwtPlot::yLeft, canvas());
    zoomer->setMousePattern(QwtEventPattern::MouseSelect1, Qt::LeftButton);
    zoomer->setRubberBand(QwtPicker::RectRubberBand);
    zoomer->setRubberBandPen(QColor(Qt::green));
    zoomer->setTrackerMode(QwtPicker::AlwaysOff);
    zoomer->setTrackerPen(QColor(Qt::white));

    mag = new PlotMagnifier(canvas());
    magX = new PlotMagnifier(canvas());
    magX->setAxisEnabled(QwtPlot::yLeft, false);
    magX->setWheelModifiers(Qt::ShiftModifier);
    magY = new PlotMagnifier(canvas());
    magY->setAxisEnabled(QwtPlot::xBottom, false);
    magY->setWheelModifiers(Qt::ControlModifier);

    picker = new PlotPicker(canvas());
    picker->setStateMachine(new QwtPickerDragPointMachine());
    picker->setTrackerPen(QPen(Qt::white));
    picker->setRubberBandPen(QPen(Qt::white));
    picker->setMousePattern(QwtEventPattern::MouseSelect1, Qt::LeftButton, Qt::ControlModifier);

    pickerPoint = new QwtPlotPicker(canvas());
    pickerPoint->setStateMachine(new QwtPickerClickPointMachine());
    //pickerPoint->setMousePattern(QwtEventPattern::MouseSelect1,Qt::LeftButton,Qt::ControlModifier);
    connect(pickerPoint, SIGNAL(selected(QPointF)), this, SLOT(pointSelected(QPointF)));

    timeCursor = new QwtPlotMarker();
    timeCursor->setLinePen(QPen(Qt::gray));
    timeCursor->setLineStyle(QwtPlotMarker::VLine);
    timeCursor->setRenderThreadCount(0);
    timeCursor->attach(this);

    // statistics of the selected time range
    pickerRange = new QwtPlotPicker(canvas());
    pickerRange->setStateMachine(new QwtPickerDragPointMachine());
    pickerRange->setMousePattern(QwtEventPattern::MouseSelect1, Qt::LeftButton, Qt::AltModifier);
    connect(pickerRange, SIGNAL(appended(QPointF)), this, SLOT(rangeStarted(QPointF)));
    connect(pickerRange, SIGNAL(moved(QPointF)), this, SLOT(rangeMoved(QPointF)));

    statsOverlay = new StatsOverlay(this);
    statsOverlay->setVisible(false);

    cursorReplotTimer.setSingleShot(true);
    cursorReplotTimer.setInterval(500);
    connect(&cursorReplotTimer, &QTimer::timeout, this, &TelemetryPlot::replot);

    canvas()->setCursor(Qt::ArrowCursor);
}

TelemetryPlot::~TelemetryPlot()
{
    delete zoomer;
    delete legend;
    delete grid;
    delete panner;
    delete panner2;
    delete mag;
    delete magX;
    delete magY;
    delete picker;
    delete pickerRange;
    delete timeCursor;
    delete statsOverlay;
}

QwtPlotCurve *TelemetryPlot::addCurve(const QString &name,
                                      const QString &descr,
                                      const QString &units,
                                      const QPen &pen)
{
    QwtPlotCurve *curve = new QwtPlotCurve();
    curve = new QwtPlotCurve();
    curve->setVisible(false);
    curve->setTitle(name);
    curve->setPen(pen);
    curve->setYAxis(QwtPlot::yLeft);
    curve->setLegendAttribute(QwtPlotCurve::LegendShowLine);
    // curve->setRenderHint(QwtPlotItem::RenderAntialiased);
    // curve->setRenderThreadCount(0);
    curve->attach(this);
    //legend tooltip
    QString s = "<html><NOBR>";
    s += "<div style='background-color: black;font-family: monospace; font-weight: bold;'><font "
         "size=+1>"
         + name + "</font></div>";
    if (!descr.isEmpty())
        s += "<div style='background-color: black;'>" + descr + "</div>";
    if (!units.isEmpty())
        s += "<div style='background-color: black;'>[" + units + "]</div>";

    QWidget *w = legend->legendWidget(itemToInfo(curve));
    if (w)
        w->setToolTip(s);
    return curve;
}

void TelemetryPlot::addEvent(double time, const QString &text, QColor color)
{
    QwtPlotMarker *m = new QwtPlotMarker();

    if (!color.isValid())
        color = QColor(Qt::darkGray);

    m->setLinePen(QPen(color));
    m->setLineStyle(QwtPlotMarker::VLine);
    m->setValue(time, 0);
    //m->setRenderThreadCount(0);

    QwtText lb(text);
    lb.setColor(Qt::white);
    lb.setBackgroundBrush(color);
    lb.setBorderRadius(4);
    m->setLabel(lb);
    m->setLabelOrientation(Qt::Vertical);
    m->setLabelAlignment(Qt::AlignBottom);

    m->setVisible(m_eventsVisible);
    m->attach(this);
    events.append(m);
}

void TelemetryPlot::resetData()
{
    m_range = QwtInterval();
    qDeleteAll(events);
    events.clear();
    const QwtPlotItemList &items = itemList(QwtPlotItem::Rtti_PlotCurve);
    for (int i = 0; i < items.size(); ++i) {
        QwtPlotCurve *curve = static_cast<QwtPlotCurve *>(items.at(i));

        if (calc_curves.contains(curve)) {
            curve->setSamples(QVector<QPointF>());
            continue;
        }

        curve->detach();
        delete curve;
    }

    for (auto it = calc_curves.begin(); it != calc_curves.constEnd(); ++it) {
        QwtPlotCurve *curve = it.key();
        if (curve) {
            showCurve(itemToInfo(curve), false);
        }
    }

    resetZoom();
    setTimeCursor(0);
}

void TelemetryPlot::resetLegend()
{
    static_cast<PlotLegend *>(legend)->clearLegenedLabels();
}

void TelemetryPlot::restoreSettings()
{
    QStringList st;
    QSettings sx;
    sx.beginGroup("plots");
    if (sx.childKeys().isEmpty()) {
        st << "est.att.roll"
           << "cmd.att.roll"
           << "est.att.pitch"
           << "cmd.att.pitch";
        st << "est.pos.altitude";
        st << "est.air.airspeed"
           << "cmd.pos.airspeed";
        st << "est.pos.vspeed";
    } else {
        st = sx.childKeys();
    }
    const QwtPlotItemList &items = itemList(QwtPlotItem::Rtti_PlotCurve);
    for (int i = 0; i < items.size(); ++i) {
        QwtPlotCurve *curve = static_cast<QwtPlotCurve *>(items.at(i));

        bool show = st.contains(curve->title().text());
        if (calc_curves.contains(curve)) {
            show = false;
        }
        showCurve(itemToInfo(curve), show);
    }
}

void TelemetryPlot::saveSettings()
{
    QSettings sx;
    sx.beginGroup("plots");
    const QwtPlotItemList &items = itemList(QwtPlotItem::Rtti_PlotCurve);
    for (int i = 0; i < items.size(); ++i) {
        auto curve = static_cast<QwtPlotCurve *>(items.at(i));
        auto s = curve->title().text();
        if (curve->isVisible())
            sx.setValue(s, true);
        else
            sx.remove(s);
    }
}

void TelemetryPlot::showCurves(bool on, const QStringList &names, bool toggle)
{
    const QwtPlotItemList &items = itemList(QwtPlotItem::Rtti_PlotCurve);
    if (toggle && on) {
        bool bAllOn = true;
        for (int i = 0; i < items.size(); ++i) {
            QwtPlotCurve *curve = static_cast<QwtPlotCurve *>(items.at(i));
            if ((!names.size()) || names.contains(curve->title().text())) {
                bAllOn &= curve->isVisible();
            }
        }
        if (bAllOn)
            on = false;
    }
    for (int i = 0; i < items.size(); ++i) {
        QwtPlotCurve *curve = static_cast<QwtPlotCurve *>(items.at(i));
        if ((!names.size()) || names.contains(curve->title().text())) {
            showCurve(itemToInfo(curve), on);
        }
    }
    resetZoom();
}

void TelemetryPlot::pointSelected(const QPointF &pos)
{
    double x = pos.x();
    if (x < 0)
        x = 0;
    timeCursor->setXValue(x);
    replot();
    emit timeCursorChanged(x);
}
void TelemetryPlot::setTimeCursor(quint64 time_ms, bool doReplot)
{
    timeCursor->setXValue(time_ms / 1000.0);
    if (doReplot)
        replot();
    else
        cursorReplotTimer.start();
}
quint64 TelemetryPlot::timeCursorValue()
{
    return timeCursor->xValue() * 1000.0;
}

void TelemetryPlot::showCurve(const QVariant &itemInfo, bool on, int index)
{
    Q_UNUSED(index);
    QwtPlotItem *item = infoToItem(itemInfo);
    if (item->isVisible() == on)
        return;
    //check if was empty plot
    bool allWereHidden = true;
    const QwtPlotItemList &items = itemList(QwtPlotItem::Rtti_PlotCurve);
    for (int i = 0; i < items.size(); ++i) {
        QwtPlotCurve *curve = static_cast<QwtPlotCurve *>(items.at(i));
        if (curve->isVisible()) {
            allWereHidden = false;
            break;
        }
    }
    //show or hide item
    item->setVisible(on);

    QwtPlotCurve *clc_curve = static_cast<QwtPlotCurve *>(item);
    if (clc_curve && calc_curves.contains(clc_curve) && on) {
        refreshCalculated(clc_curve);
    }

    emit itemVisibleChanged(item);
    //update legend
    QwtLegendLabel *w = qobject_cast<QwtLegendLabel *>(legend->legendWidget(itemInfo));
    if (w)
        w->setChecked(item->isVisible());
    //update zoom or replot
    if (allWereHidden)
        resetZoom();
    else
        replot();
}

void TelemetryPlot::refreshCalculated(QwtPlotCurve *curve_calc)
{
    bool ok;
    QString exp_calc = calc_curves.value(curve_calc);
    if (exp_calc.isEmpty())
        exp_calc = "est.att.yaw-est.calc.bearing";
    QString exp = QInputDialog::getText(nullptr,
                                        tr("Calculated field"),
                                        tr("JavaScript expression:"),
                                        QLineEdit::Normal,
                                        exp_calc,
                                        &ok);
    if (ok == false) {
        curve_calc->setVisible(false);
        return;
    }
    calc_curves[curve_calc] = exp;

    //fill internal data
    QVector<QPointF> points;
    QJSEngine engine;

    const QwtPlotItemList &items = itemList(QwtPlotItem::Rtti_PlotCurve);

    QVector<const QwtSeriesData<QPointF> *> fdata;
    QVector<QString> fnames;
    quint64 tcnt = 0;
    QVector<quint64> fpidx;
    for (int i = 0; i < items.size(); ++i) {
        QwtPlotCurve *curve = static_cast<QwtPlotCurve *>(items.at(i));
        fdata.append(curve->data());
        fnames.append(curve->title().text());
        fpidx.append(0);
        engine.globalObject().setProperty(fnames.last(),
                                          curve->data()->size() > 0 ? curve->data()->sample(0).y()
                                                                    : 0);
        tcnt += curve->data()->size();
    }
    quint64 cnt = 0;
    double vcalc = 0;
    double tMax = 0;
    while (tcnt) {
        setProgress(cnt * 100 / tcnt);
        //find earliest time value
        double t = -1;
        for (int i = 0; i < fpidx.size(); ++i) {
            const QwtSeriesData<QPointF> *points = fdata.at(i);
            quint64 didx = fpidx.at(i);
            if (didx >= points->size())
                continue;
            const double &ft = points->sample(didx).x();
            if (t >= 0 && ft >= t)
                continue;
            t = ft;
        }
        if (t < 0)
            break; //done
        if (tMax < t)
            tMax = t;
        engine.globalObject().setProperty("time", t);
        //update fields with time=t
        for (int i = 0; i < fpidx.size(); ++i) {
            const QwtSeriesData<QPointF> *points = fdata.at(i);
            quint64 didx = fpidx.at(i);
            if (didx >= points->size())
                continue;
            const QPointF &p = points->sample(didx);
            if (p.x() != t)
                continue;
            QString s = fnames.at(i);
            QJSValue obj = engine.globalObject();
            while (1) {
                int i = s.indexOf('.');
                if (i < 0) {
                    obj.setProperty(s, p.y());
                    break;
                }
                const QString &ps = s.left(i);
                QJSValue v = obj.property(ps);
                if (!v.isObject()) {
                    v = engine.newObject();
                    obj.setProperty(ps, v);
                }
                obj = v;
                s.remove(0, i + 1);
            }
            fpidx[i] = didx + 1;
            cnt++;
        }
        double v = engine.evaluate(exp).toNumber();
        if (v == vcalc)
            continue;
        vcalc = v;
        if (points.size() > 0 && (t - points.last().x()) > 0.5) {
            //extrapolate unchanged value tail-1ms
            points.append(QPointF(t - 0.001, points.last().y()));
        }
        points.append(QPointF(t, v));
    }
    //final data tail at max time
    if ((!points.isEmpty()) && points.last().x() < tMax) {
        points.append(QPointF(tMax, points.last().y()));
    }

    setProgress(0);
    //install data
    curve_calc->setData(new QwtPointSeriesData(points));
    curve_calc->setVisible(true);

    //resetZoom();
    replot();
}

void TelemetryPlot::setProgress(int v)
{
    if (m_progress == v)
        return;
    m_progress = v;
    emit progressChanged(v);
}

void TelemetryPlot::resetZoom()
{
    double vmax = 0.0, vmin = 0.0, tMax = 0.0;
    const QwtPlotItemList &items = itemList(QwtPlotItem::Rtti_PlotCurve);
    for (int i = 0; i < items.size(); ++i) {
        QwtPlotCurve *curve = static_cast<QwtPlotCurve *>(items.at(i));
        for (size_t i2 = 0; i2 < curve->data()->size(); ++i2) {
            const QPointF &p = curve->data()->sample(i2);
            if (tMax < p.x())
                tMax = p.x();
            double v = p.y();
            if (curve->isVisible() && (!std::isnan(v)) && (!std::isinf(v))) {
                if (vmax < v)
                    vmax = v;
                if (vmin > v)
                    vmin = v;
            }
        }
    }
    //reset zoom
    double margin = std::abs(vmax - vmin) * 0.1;
    vmin -= margin;
    vmax += margin;
    QRectF r(0, vmin, tMax, vmax - vmin);
    zoomer->zoom(r);
    zoomer->setZoomBase(r);
    //zoomer->zoom(r);
    //zoomer->setZoomBase(r);
    //QCoreApplication::processEvents();
    replot();
}

void TelemetryPlot::copyFromPlot(TelemetryPlot *plot)
{
    //collect curves
    const QwtPlotItemList &items = plot->itemList(QwtPlotItem::Rtti_PlotCurve);
    for (int i = 0; i < items.size(); ++i) {
        QwtPlotCurve *c = static_cast<QwtPlotCurve *>(items.at(i));
        QwtPlotCurve *curve = new QwtPlotCurve();
        curve = new QwtPlotCurve();
        curve->setVisible(false);
        curve->setTitle(c->title());
        curve->setPen(c->pen());
        curve->setYAxis(QwtPlot::yLeft);
        curve->setLegendAttribute(QwtPlotCurve::LegendShowLine);
        curve->setRenderHint(QwtPlotItem::RenderAntialiased);
        curve->setData(new QwtPointSeriesData(((QwtPointSeriesData *) c->data())->samples()));
        //curve->setData(c->data());
        curve->attach(this);
        showCurve(itemToInfo(curve), c->isVisible());

        if (plot->calc_curves.contains(c)) {
            push_calc_curve(curve);
        }
    }
    //events
    for (int i = 0; i < plot->events.size(); ++i) {
        addEvent(plot->events.at(i)->value().x(),
                 plot->events.at(i)->label().text(),
                 plot->events.at(i)->label().backgroundBrush().color());
    }

    //same zoom
    setAxisScaleDiv(QwtPlot::yLeft, plot->axisScaleDiv(QwtPlot::yLeft));
    setAxisScaleDiv(QwtPlot::xBottom, plot->axisScaleDiv(QwtPlot::xBottom));
    zoomer->setZoomBase(plot->zoomer->zoomBase());

    setEventsVisible(plot->eventsVisible());

    m_range = plot->m_range;
    setStatsVisible(plot->statsVisible());

    replot();
}

bool TelemetryPlot::eventsVisible() const
{
    return m_eventsVisible;
}
void TelemetryPlot::setEventsVisible(bool v)
{
    if (m_eventsVisible == v)
        return;
    m_eventsVisible = v;
    for (int i = 0; i < events.size(); ++i) {
        events.at(i)->setVisible(v);
    }
    replot();
}

bool TelemetryPlot::statsVisible() const
{
    return m_statsVisible;
}
void TelemetryPlot::setStatsVisible(bool v)
{
    if (m_statsVisible == v)
        return;
    m_statsVisible = v;
    statsOverlay->setVisible(v);
    if (v)
        updateStats();
    emit statsVisibleChanged(v);
}

void TelemetryPlot::rangeStarted(const QPointF &pos)
{
    // click drops previous selection immediately
    m_rangeStart = pos.x();
    if (!m_range.isValid())
        return;
    m_range = QwtInterval();
    if (m_statsVisible)
        updateStats();
}

void TelemetryPlot::rangeMoved(const QPointF &pos)
{
    // stats follow the mouse while dragging, a few pixels are still a click
    const QwtScaleMap map = canvasMap(QwtPlot::xBottom);
    QwtInterval range;
    if (std::abs(map.transform(pos.x()) - map.transform(m_rangeStart)) >= 3)
        range = QwtInterval(m_rangeStart, pos.x()).normalized();

    if (range == m_range)
        return;
    m_range = range;

    if (m_range.isValid() && !m_statsVisible)
        setStatsVisible(true);
    else if (m_statsVisible)
        updateStats();
}

void TelemetryPlot::replot()
{
    QwtPlot::replot();
    // stats follow zoom and data changes
    if (m_statsVisible)
        updateStats();
}

struct CurveStats
{
    double min;
    double max;
    double avg;
    double std;
};

// Telemetry values are recorded on change and hold until the next sample,
// so avg and std are weighted by time to not overrate frequently updated parts.
static bool curveStats(const QVector<QPointF> &pts, const QwtInterval &range, CurveStats &st)
{
    const double t0 = range.minValue();
    const double t1 = range.maxValue();

    // last sample at or before t0 holds the value at the range start
    auto it = std::upper_bound(pts.cbegin(), pts.cend(), t0, [](double t, const QPointF &p) {
        return t < p.x();
    });
    if (it == pts.cbegin())
        return false;
    --it;

    double vmin = std::numeric_limits<double>::infinity();
    double vmax = -vmin;

    // time weighted sums of deviations from the first value to keep precision
    double wsum = 0, s1 = 0, s2 = 0, ref = 0;
    const auto add = [&](double v, double dt) {
        if (!(dt > 0) || !std::isfinite(v))
            return;
        if (wsum == 0)
            ref = v;
        vmin = std::min(vmin, v);
        vmax = std::max(vmax, v);
        const double d = v - ref;
        wsum += dt;
        s1 += d * dt;
        s2 += d * d * dt;
    };

    double t = t0;
    double v = it->y();
    for (++it; it != pts.cend() && it->x() <= t1; ++it) {
        add(v, it->x() - t);
        t = std::max(t, it->x());
        v = it->y();
    }
    add(v, t1 - t);

    if (!(wsum > 0))
        return false;

    const double mean = s1 / wsum;
    st = {vmin, vmax, ref + mean, std::sqrt(std::max(s2 / wsum - mean * mean, 0.0))};
    return true;
}

// decimals to display the spread of values
static int statsPrecision(double min, double max)
{
    double span = max - min;
    if (!(span > 0)) {
        if (min == std::trunc(min))
            return 0;
        span = std::abs(min);
    }
    if (!std::isfinite(span))
        return 0;
    return std::clamp(4 - static_cast<int>(std::floor(std::log10(span))), 0, 6);
}

void TelemetryPlot::updateStats()
{
    // selected or visible time range
    const QwtInterval range = m_range.isValid() ? m_range
                                                : axisInterval(QwtPlot::xBottom).normalized();

    QList<StatsOverlay::Row> rows;
    QwtInterval dataRange;
    int width = 0;
    const QwtPlotItemList &items = itemList(QwtPlotItem::Rtti_PlotCurve);
    for (int i = 0; i < items.size(); ++i) {
        QwtPlotCurve *curve = static_cast<QwtPlotCurve *>(items.at(i));
        if (!curve->isVisible())
            continue;
        const auto series = dynamic_cast<const QwtPointSeriesData *>(curve->data());
        const QVector<QPointF> pts = series ? series->samples() : QVector<QPointF>();

        // format by all values of the curve, so decimals and width don't change with range
        double ymin = std::numeric_limits<double>::infinity();
        double ymax = -ymin;
        for (const auto &p : pts) {
            ymin = std::min(ymin, p.y()); // NaN skipped
            ymax = std::max(ymax, p.y());
        }
        const int prec = ymin <= ymax ? statsPrecision(ymin, ymax) : 0;
        const double zero = 0.5 * std::pow(10.0, -prec);
        const auto text = [prec, zero](double v) {
            return QString::number(std::abs(v) < zero ? 0.0 : v, 'f', prec); // avoid "-0.00"
        };

        // keep the row when no values in range to not resize the table
        StatsOverlay::Row row{curve->title().text(), curve->pen().color(), {"-", "-", "-", "-"}};
        CurveStats st;
        const QwtInterval r = pts.isEmpty()
                                  ? QwtInterval()
                                  : range & QwtInterval(pts.first().x(), pts.last().x());
        if (r.isValid() && curveStats(pts, r, st)) {
            dataRange |= r;
            row.values = QStringList{text(st.min), text(st.max), text(st.avg), text(st.std)};
        }
        QStringList widest = row.values;
        if (ymin <= ymax)
            widest << text(ymin) << text(ymax);
        for (const auto &s : widest)
            width = std::max(width, static_cast<int>(s.size()));
        rows.append(row);
    }
    for (auto &row : rows) {
        for (auto &s : row.values)
            s = s.rightJustified(width);
    }

    QString title;
    if (!rows.isEmpty()) {
        const QwtInterval t = dataRange.isValid() ? dataRange : range;
        title = QString("%1 - %2 (%3)").arg(timeText(t.minValue()),
                                            timeText(t.maxValue()),
                                            timeText(t.width()));
    }
    statsOverlay->setStats(m_range, title, rows);
}

static const int statsPadding = 6;
static const int statsSpacing = 16;

// width by chars count to not depend on digits in proportional fonts
static int statsTextWidth(const QFontMetrics &fm, const QString &s)
{
    int charWidth = 0;
    for (const QChar c : QStringLiteral("0123456789-.:() "))
        charWidth = std::max(charWidth, fm.horizontalAdvance(c));
    return std::max(fm.horizontalAdvance(s), static_cast<int>(s.size()) * charWidth);
}

StatsOverlay::StatsOverlay(TelemetryPlot *plot)
    : QwtWidgetOverlay(plot->canvas())
    , m_plot(plot)
    , m_header({tr("min"), tr("max"), tr("avg"), tr("std")})
{
    setMaskMode(QwtWidgetOverlay::NoMask);
    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
}

void StatsOverlay::setStats(const QwtInterval &selection,
                            const QString &title,
                            const QList<Row> &rows)
{
    m_selection = selection;
    m_title = title;
    m_rows = rows;

    const QFontMetrics fm(font());
    QFont bold(font());
    bold.setBold(true);

    m_nameWidth = 0;
    m_valueWidth = 0;
    for (const auto &s : m_header)
        m_valueWidth = std::max(m_valueWidth, fm.horizontalAdvance(s));
    for (const auto &row : m_rows) {
        m_nameWidth = std::max(m_nameWidth, fm.horizontalAdvance(row.name));
        for (const auto &s : row.values)
            m_valueWidth = std::max(m_valueWidth, statsTextWidth(fm, s));
    }
    const int w = std::max(m_nameWidth + static_cast<int>(m_header.size()) * (statsSpacing + m_valueWidth),
                           statsTextWidth(QFontMetrics(bold), m_title));
    const int h = static_cast<int>(m_rows.size() + 2) * fm.height();
    m_tableSize = QSize(w + 2 * statsPadding, h + 2 * statsPadding);

    update();
}

QRect StatsOverlay::tableRect() const
{
    if (m_rows.isEmpty())
        return QRect();
    // top right corner of canvas
    const QRect cr = parentWidget()->contentsRect();
    return QRect(QPoint(cr.right() - 5 - m_tableSize.width(), cr.top() + 5), m_tableSize);
}

void StatsOverlay::drawOverlay(QPainter *painter) const
{
    const QRect cr = parentWidget()->contentsRect();

    // selected range
    if (m_selection.isValid()) {
        const QwtScaleMap map = m_plot->canvasMap(QwtPlot::xBottom);
        const double x0 = map.transform(m_selection.minValue());
        const double x1 = map.transform(m_selection.maxValue());
        const QRectF band = QRectF(QPointF(x0, cr.top()), QPointF(x1, cr.bottom())).normalized();
        const QColor color(0, 150, 255);
        painter->fillRect(band, QColor(color.red(), color.green(), color.blue(), 40));
        painter->setPen(QPen(color, 0, Qt::DashLine));
        painter->drawLine(band.topLeft(), band.bottomLeft());
        painter->drawLine(band.topRight(), band.bottomRight());
    }

    const QRect box = tableRect();
    if (box.isEmpty())
        return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QColor(100, 100, 100));
    painter->setBrush(QColor(0, 0, 0, 180));
    painter->drawRoundedRect(QRectF(box).adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
    painter->restore();

    const int lh = QFontMetrics(font()).height();
    const int left = box.left() + statsPadding;
    int y = box.top() + statsPadding;

    QFont bold(font());
    bold.setBold(true);
    painter->setFont(bold);
    painter->setPen(Qt::white);
    painter->drawText(QRect(left, y, box.width() - 2 * statsPadding, lh),
                      Qt::AlignLeft | Qt::AlignVCenter,
                      m_title);
    painter->setFont(font());

    const auto drawRow = [&](const QString &name,
                             const QColor &nameColor,
                             const QStringList &values,
                             const QColor &color) {
        y += lh;
        painter->setPen(nameColor);
        painter->drawText(QRect(left, y, m_nameWidth, lh), Qt::AlignLeft | Qt::AlignVCenter, name);
        painter->setPen(color);
        int x = left + m_nameWidth;
        for (const auto &s : values) {
            x += statsSpacing;
            painter->drawText(QRect(x, y, m_valueWidth, lh), Qt::AlignRight | Qt::AlignVCenter, s);
            x += m_valueWidth;
        }
    };
    drawRow(QString(), Qt::white, m_header, QColor(160, 160, 160));
    for (const auto &row : m_rows)
        drawRow(row.name, row.color, row.values, Qt::white);
}

void TelemetryPlot::mouseReleaseEvent(QMouseEvent *event)
{
    QwtPlot::mouseReleaseEvent(event);
    if (event->button() == Qt::RightButton)
        resetZoom();
    //qDebug()<<event;
}

QwtText PlotPicker::trackerText(const QPoint &pos) const
{
    double t = plot()->invTransform(QwtPlot::xBottom, pos.x());
    QString s = "<html><NOBR><table>";
    s += "<tr><td colspan=2 align=left style='font-family: monospace; font-weight: "
         "bold;'><PRE><font size=+4>";
    s += QTime(0, 0).addSecs(t).toString("hh:mm:ss"); //+QString::number(t)+" sec)";
    s += "</font></PRE></td></tr>";
    const QwtPlotItemList &items = plot()->itemList(QwtPlotItem::Rtti_PlotCurve);
    for (int i = 0; i < items.size(); ++i) {
        QwtPlotCurve *curve = static_cast<QwtPlotCurve *>(items.at(i));
        if (!curve->isVisible())
            continue;
        double v = sampleValue(curve, t);
        QColor c = curve->pen().color();
        if (curve->pen().style() != Qt::SolidLine)
            c = c.darker();
        //QString units;
        //Fact *fact=Fleet::instance()->f_local->f_mandala->factByName(name);
        //if(fact)units=fact->units();
        s += "<tr><td align=right><font size=+2 color=" + c.name() + ">" + curve->title().text()
             + "</font>&nbsp;&nbsp;</td><td align=left><font size=+2>"
             + QString("%1").arg(v, 0, 'f', 2) + "</font></td></tr>";
    }
    s += "</table>";
    return QwtText(s);
}

double PlotPicker::sampleValue(const QwtPlotCurve *curve, double t) const
{
    if (curve->data()->size() < 50) {
        for (size_t i = 0; i < curve->data()->size(); ++i) {
            if (curve->data()->sample(i).x() >= t) {
                return curve->data()->sample(i).y();
            }
        }
        return 0;
    }
    size_t ts = curve->data()->size() / 2;
    size_t tx = ts;
    bool bFound = false, bFwd = false;
    while (1) {
        const QPointF &p = curve->data()->sample(tx);
        double vx = p.x();
        ts >>= 1;
        if (ts == 0) {
            if (bFound)
                return p.y();
            //if(vx>t && tx>0)ts=1;
            //else return p.y();
            ts = 1;
        }
        if (vx < t) {
            tx += ts;
            if (tx >= curve->data()->size()) {
                tx = curve->data()->size() - 1;
                if (ts == 1)
                    bFound = true;
            }
            bFwd = true;
        } else if (vx > t) {
            if (ts == 1 && bFwd == true)
                bFound = true;
            if (tx > ts)
                tx -= ts;
            else {
                tx = 0;
                if (ts == 1)
                    bFound = true;
            }
            bFwd = false;
        } else
            return p.y();
    }
}

void LegendItem::paintEvent(QPaintEvent *e)
{
    const QRect cr = contentsRect();
    QPainter painter(this);
    painter.setClipRegion(e->region());
    if (isChecked()) {
        painter.save();
        painter.setBrush(QColor(50, 50, 50));
        painter.setPen(QPen(QColor(100, 100, 100), 1, Qt::DotLine));
        painter.drawRoundedRect(cr.adjusted(0, 0, -1, -1), 2, 2);
        painter.restore();
    }
    painter.save();
    painter.setClipRect(cr);
    drawContents(&painter);
    QRect identRect = cr;
    identRect.setX(identRect.x() + margin());
    identRect.setSize(icon().size());
    identRect.setWidth(10);
    identRect.moveCenter(QPoint(identRect.center().x(), cr.center().y()));
    painter.drawPixmap(identRect, icon());
    painter.restore();
}

void PlotMagnifier::widgetMouseMoveEvent(QMouseEvent *mouseEvent)
{
    mwPos = mouseEvent->pos();
    QwtPlotMagnifier::widgetMouseMoveEvent(mouseEvent);
}
void PlotMagnifier::rescale(double factor)
{
    factor = qAbs(factor);
    if (factor == 1.0 || factor == 0.0)
        return;
    bool doReplot = false;
    QwtPlot *plt = plot();
    const bool autoReplot = plt->autoReplot();
    plt->setAutoReplot(false);
    for (int axisId = 0; axisId < QwtPlot::axisCnt; axisId++) {
        const QwtScaleDiv &scaleDiv = plt->axisScaleDiv(axisId);
        if (isAxisEnabled(axisId)) {
            // get the range of the axis
            double min_value = scaleDiv.lowerBound();
            double max_value = scaleDiv.upperBound();
            // convert to screen coordinates
            QwtScaleMap mapper = plt->canvasMap(axisId);
            double min_pixel = mapper.transform(min_value);
            double max_pixel = mapper.transform(max_value);
            // get the mouse position on this axis
            double center;
            if (axisId == QwtPlot::yLeft || axisId == QwtPlot::yRight)
                center = mwPos.y();
            else
                center = mwPos.x();
            // use the mouse as the center when we can,
            // if its -1, then its a keyboard event. use the window center.
            if (center == -1.0)
                center = min_pixel + (max_pixel - min_pixel) / 2;
            // convert back to real values
            min_value = mapper.invTransform(center - (center - min_pixel) * factor);
            max_value = mapper.invTransform(center + (max_pixel - center) * factor);
            // set the new range
            plt->setAxisScale(axisId, min_value, max_value);
            //plt->setAxisScale( axisId, center - width_2, center + width_2 );
            doReplot = true;
        }
    }
    plt->setAutoReplot(autoReplot);
    if (doReplot)
        plt->replot();
}

PlotLegend::PlotLegend(QWidget *parent)
    : QwtLegend(parent)
{
    filter_le = new QLineEdit();
    filter_le->setPlaceholderText(tr("Filter..."));
    connect(filter_le, SIGNAL(textChanged(QString)), this, SLOT(onFilter(QString)));

    static_cast<QVBoxLayout *>(layout())->insertWidget(0, filter_le);
}

QWidget *PlotLegend::createWidget(const QwtLegendData &data) const
{
    Q_UNUSED(data)
    LegendItem *w = new LegendItem();
    w->setItemMode(defaultItemMode());
    w->setSpacing(3);
    w->setMargin(0);
    const_cast<PlotLegend *>(this)->legendLabels.append(w);

    connect(w, SIGNAL(clicked()), this, SLOT(itemClicked()));
    connect(w, SIGNAL(checked(bool)), this, SLOT(itemChecked(bool)));
    return w;
}

void PlotLegend::clearLegenedLabels()
{
    legendLabels.clear();
}

void PlotLegend::onFilter(QString text)
{
    QLayout *contentsLayout = contentsWidget()->layout();

    if (!contentsLayout) {
        return;
    }

    text.remove(' ');

    for (const auto &ptr : legendLabels) {
        if (!ptr) {
            continue;
        }

        LegendItem *label = ptr.data();
        const bool match = label->text().text().contains(text, Qt::CaseInsensitive);

        if (match) {
            if (contentsLayout->indexOf(label) < 0) {
                contentsLayout->addWidget(label);
            }
            label->setVisible(true);
        } else {
            contentsLayout->removeWidget(label);
            label->setVisible(false);
        }
    }
}
