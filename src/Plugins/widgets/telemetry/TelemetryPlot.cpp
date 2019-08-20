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
#include "TelemetryPlot.h"
#include <QJSEngine>
#include <QtGui>
//=============================================================================
TelemetryPlot::TelemetryPlot(QWidget *parent)
    : QwtPlot(parent)
    , calc(nullptr)
    , m_progress(0)
    , m_eventsVisible(false)
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
    panner->setMouseButton(Qt::MidButton);
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

    cursorReplotTimer.setSingleShot(true);
    cursorReplotTimer.setInterval(500);
    connect(&cursorReplotTimer, &QTimer::timeout, this, &TelemetryPlot::replot);

    canvas()->setCursor(Qt::ArrowCursor);
}
//=============================================================================
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
    delete timeCursor;
}
//=============================================================================
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
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
    curve->setRenderThreadCount(0);
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
//=============================================================================
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
//=============================================================================
void TelemetryPlot::resetData()
{
    qDeleteAll(events);
    events.clear();
    const QwtPlotItemList &items = itemList(QwtPlotItem::Rtti_PlotCurve);
    for (int i = 0; i < items.size(); ++i) {
        QwtPlotCurve *curve = static_cast<QwtPlotCurve *>(items.at(i));
        curve->setSamples(QVector<QPointF>());
    }
    if (calc)
        showCurve(itemToInfo(calc), false);
    resetZoom();
    setTimeCursor(0);
}
//=============================================================================
void TelemetryPlot::restoreSettings()
{
    if (!QSettings().contains("plots"))
        QSettings().setValue("plots",
                             QStringList() << "pitch"
                                           << "cmd_airspeed"
                                           << "cmd_roll"
                                           << "cmd_pitch"
                                           << "altitude"
                                           << "cmd_altitude"
                                           << "roll"
                                           << "airspeed");
    QStringList sps = QSettings().value("plots").toStringList();
    const QwtPlotItemList &items = itemList(QwtPlotItem::Rtti_PlotCurve);
    for (int i = 0; i < items.size(); ++i) {
        QwtPlotCurve *curve = static_cast<QwtPlotCurve *>(items.at(i));
        showCurve(itemToInfo(curve), (curve == calc) ? false : sps.contains(curve->title().text()));
    }
}
//=============================================================================
void TelemetryPlot::saveSettings()
{
    QStringList sps;
    const QwtPlotItemList &items = itemList(QwtPlotItem::Rtti_PlotCurve);
    for (int i = 0; i < items.size(); ++i) {
        QwtPlotCurve *curve = static_cast<QwtPlotCurve *>(items.at(i));
        if (curve->isVisible())
            sps.append(curve->title().text());
    }
    QSettings().setValue("plots", sps);
}
//=============================================================================
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
//=============================================================================
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
//=============================================================================
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
    if (item == calc && on)
        refreshCalculated();
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
//=============================================================================
void TelemetryPlot::refreshCalculated()
{
    bool ok;
    if (expCalc.isEmpty())
        expCalc = "altitude+down";
    QString exp = QInputDialog::getText(nullptr,
                                        tr("Calculated field"),
                                        tr("JavaScript expression:"),
                                        QLineEdit::Normal,
                                        expCalc,
                                        &ok);
    if (ok == false) {
        calc->setVisible(false);
        return;
    }
    expCalc = exp;

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
            engine.globalObject().setProperty(fnames.at(i), p.y());
            fpidx[i] = didx + 1;
            cnt++;
        }
        double v = engine.evaluate(expCalc).toNumber();
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
    calc->setData(new QwtPointSeriesData(points));
    calc->setVisible(true);
    //resetZoom();
    replot();
}
//=============================================================================
void TelemetryPlot::setProgress(int v)
{
    if (m_progress == v)
        return;
    m_progress = v;
    emit progressChanged(v);
}
//=============================================================================
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
//=============================================================================
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
        if (c == plot->calc)
            calc = curve;
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

    replot();
}
//=============================================================================
//=============================================================================
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
//=============================================================================
void TelemetryPlot::mouseReleaseEvent(QMouseEvent *event)
{
    QwtPlot::mouseReleaseEvent(event);
    if (event->button() == Qt::RightButton)
        resetZoom();
    //qDebug()<<event;
}
//=============================================================================
//=============================================================================
//=============================================================================
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
        //Fact *fact=Vehicles::instance()->f_local->f_mandala->factByName(name);
        //if(fact)units=fact->units();
        s += "<tr><td align=right><font size=+2 color=" + c.name() + ">" + curve->title().text()
             + "</font>&nbsp;&nbsp;</td><td align=left><font size=+2>"
             + QString().sprintf("%.2f", v) + "</font></td></tr>";
    }
    s += "</table>";
    return QwtText(s);
}
//=============================================================================
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
    return 0;
}
//=============================================================================
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
    identRect.moveCenter(QPoint(identRect.center().x(), cr.center().y()));
    painter.drawPixmap(identRect, icon());
    painter.restore();
}
//=============================================================================
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
            if (center == -1)
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
//=============================================================================
PlotLegend::PlotLegend(QWidget *parent)
    : QwtLegend(parent)
{}
//=============================================================================
QWidget *PlotLegend::createWidget(const QwtLegendData &data) const
{
    Q_UNUSED(data);
    QwtLegendLabel *w = new LegendItem();
    w->setItemMode(defaultItemMode());
    w->setSpacing(3);
    w->setMargin(0);
    connect(w, SIGNAL(clicked()), this, SLOT(itemClicked()));
    connect(w, SIGNAL(checked(bool)), this, SLOT(itemChecked(bool)));
    return w;
}
//=============================================================================
