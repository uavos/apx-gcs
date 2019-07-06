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
#include "MandalaApp.h"
#include "MandalaTree.h"
#include "MandalaTreeVehicle.h"
#include "MandalaTreeField.h"
//=============================================================================
MandalaApp::MandalaApp(QObject *parent)
    : MandalaTree(parent)
    , m_current(new MandalaTreeVehicle(new MandalaTree(parent), NULL))
    , m_current_set(NULL)
{
    js = new QJSEngine(this);
    js->installExtensions(QJSEngine::TranslationExtension | QJSEngine::ConsoleExtension);
    jsObj = syncJS(js, this, js->globalObject());

    if (!qApp->property("Mandala").isValid()) {
        //only one global mandala property
        qApp->setProperty("Mandala", qVariantFromValue(this));
    }

    prefs = new AppProperties(this);
    qApp->setProperty(prefs->objectName().toUtf8().data(), qVariantFromValue(prefs));

    //add app object
    QJSValue jsApp = js->newQObject(qApp);
    js->globalObject().setProperty("app", jsApp);
    //add all app child QObjects
    foreach (QString s, qApp->dynamicPropertyNames()) {
        QVariant v = qApp->property(s.toUtf8().data());
        if (v.canConvert(QMetaType::QObjectStar))
            jsApp.setProperty(s, js->newQObject(v.value<QObject *>()));
    }

    syncJS(js, m_current, jsObj); //m.current.<members>
    syncJScurrent(js, m_current); //<alias>=value
    foreach (MandalaTree *i, m_current->childItems)
        syncJS(js, i, js->globalObject()); //<members>.<members>...

    //system functions and objects
    add_js("help()", QObject::tr("print commands help"), "m.help();");
    add_js("req(n)", QObject::tr("request var n from UAV"), "mandala[n].request();");
    add_js("send(n)", QObject::tr("send var n to UAV"), "mandala[n].send();");
    add_js("nodes()", QObject::tr("rescan bus nodes"), "print('nodes:');mandala.req_nodes();");
    add_js("nstat()",
           QObject::tr("print nodes status"),
           "print('nodes statistics:');mandala.req_nstat();");
    add_js("serial(p,v)",
           QObject::tr("send data v to serial port ID p"),
           "mandala.scr_serial(p,v);");
    add_js("can(i,e,v)",
           QObject::tr("send data v to CAN ID i with IDE e"),
           "mandala.scr_can(i,e,v);");
    add_js("vmexec(f)", QObject::tr("execute function on VMs"), "mandala.scr_vmexec(f);");
    add_js("sleep(n)", QObject::tr("sleep n milliseconds"), "mandala.scr_sleep(n);");
    add_js("next()", QObject::tr("switch to next vehicle"), "mandala.global.changeCurrentNext();");
    add_js("prev()",
           QObject::tr("switch to previous vehicle"),
           "mandala.global.changeCurrentPrev();");
    //some helper functions
    add_js("trigger(v,a,b)",
           QObject::tr("trigger value of v to a or b"),
           "if(v==a)return b; else return a;");
    add_js("bound(v)",
           QObject::tr("wrap angle -180..+180"),
           "while(v>=180)v-=360;while(v<-180)v+=360;return v;");
    add_js("ls(a,b)",
           QObject::tr("print members of type b for scope a"),
           "for(var i in a)if(typeof(a[i])==b || !b)print(i+\" - \"+typeof(a[i]));");
    add_js("vars(a)",
           QObject::tr("print variables for scope a"),
           "if(arguments.length==0)a=this;for(var i in "
           "a)if(typeof(a[i])=='number')print(i+\"=\"+a[i]);");
    add_js("func(a)",
           QObject::tr("print functions for scope a"),
           "if(arguments.length==0)a=this;for(var i in a)if(typeof(a[i])=='function')print(i);");
    //predefined commands
    add_js("ahrs()", QObject::tr("reset AHRS"), "req('roll');");
    add_js("zrc()",
           QObject::tr("reset pilot controls"),
           "rc_roll=0;rc_pitch=0;rc_throttle=0;rc_yaw=0;");
    add_js("zps()", QObject::tr("reset barometric altitude on ground"), "altps_gnd=0;");
    add_js("ned()",
           QObject::tr("reset local GPS coordinates"),
           "home_lat=gps_lat;home_lon=gps_lon;home_hmsl=gps_hmsl;");
    add_js("hmsl()", QObject::tr("reset local GPS altitude"), "home_hmsl=gps_hmsl;");

    // script include file (default)
    QFile jsFile(QDir(prefs->configDir()).filePath("default.js"));
    if (jsFile.open(QIODevice::ReadOnly)) {
        QTextStream stream(&jsFile);
        QString contents = stream.readAll();
        jsFile.close();
        jsexec(contents);
    }
    // script include file (user commands)
    QFile jsFile2(QDir(prefs->scrDir()).filePath("gcu.js"));
    if (jsFile2.open(QIODevice::ReadOnly)) {
        QTextStream stream(&jsFile2);
        QString contents = stream.readAll();
        jsFile2.close();
        jsexec(contents);
    }

    onlineTimer.setSingleShot(true);
    onlineTimer.setInterval(7000);
    connect(&onlineTimer, &QTimer::timeout, this, [=]() { updateOnline(false); });
    connect(this, &MandalaTree::dlcntChanged, this, [=]() { updateOnline(true); });

    // create LOCAL vehicle
    IDENT::_ident ident;
    memset(&ident, 0, sizeof(ident));
    strncat(ident.callsign, "LOCAL", sizeof(ident.callsign));
    setCurrent(addVehicle(&ident));
}
MandalaApp::~MandalaApp() {}
//=============================================================================
//=============================================================================
MandalaTreeVehicle *MandalaApp::addVehicle(IDENT::_ident *ident)
{
    if ((!(ident && ident->squawk)) && size() > 1)
        return NULL; //one LOCAL allowed
    //check if already exists
    MandalaTreeVehicle *m = NULL;
    foreach (MandalaTree *i, childItems) {
        m = static_cast<MandalaTreeVehicle *>(i);
        if (m->index() == ident->squawk)
            break;
        if (m->name() == QString(ident->callsign)) {
            qDebug("%s: %s (%.4X)",
                   tr("Duplicate callsign").toUtf8().data(),
                   ident->callsign,
                   ident->squawk);
        }
        m = NULL;
    }
    if (m)
        return m;
    //create new
    m = new MandalaTreeVehicle(this, ident);
    syncJS(js, m, js->newQObject(m)); //update for struct
    emit vehicleAdded(m);
    if (!m_current_set)
        setCurrent(m);
    return m;
}
//=============================================================================
QJSValue MandalaApp::syncJS(QJSEngine *e, MandalaTree *m, QJSValue parent) //recursive
{
    QJSValue js_mi = e->newQObject(m);
    parent.setProperty(m->name(), js_mi);
    /* QString s=m->path(2);
  if(s.contains('.')){
    QString sobj=s.left(s.lastIndexOf('.'));
    QString sprop=s.mid(s.lastIndexOf('.')+1);
    s=QString("%1.__defineSetter__('%2', function(v){ print('Error: can't write to %1'); });").arg(sobj).arg(sprop);
    e->evaluate(s);
    qDebug()<<s;
  }*/
    foreach (MandalaTree *i, m->childItems)
        syncJS(e, i, js_mi);
    return js_mi;
}
void MandalaApp::syncJScurrent(QJSEngine *e, MandalaTree *m) //recursive
{
    if (m->itype() == MandalaTree::it_field) {
        MandalaTreeField *fi = static_cast<MandalaTreeField *>(m);
        //aliases
        if (!fi->alias().isEmpty()) {
            //e->evaluate(QString("m.__defineGetter__('%1', function(){ return %2; });").arg(fi->alias()).arg(fi->path()));
            e->evaluate(QString("this.__defineGetter__('%1', function(){ return %2.value; });")
                            .arg(fi->alias())
                            .arg(fi->path()));
            e->evaluate(QString("this.__defineSetter__('%1', function(v){ %2.value=v; });")
                            .arg(fi->alias())
                            .arg(fi->path()));
        }
        //constants
        for (int i = 0; i < fi->opts().size(); i++) {
            e->evaluate(QString("this.__defineGetter__('%1_%2', function(){ return %3; });")
                            .arg(fi->name())
                            .arg(fi->opts().at(i))
                            .arg(i));
            //e->globalObject().setProperty(QString("%1_%2").arg(fi->name()).arg(fi->opts().at(i)),i);
        }
    }
    foreach (MandalaTree *i, m->childItems)
        syncJScurrent(e, i);
}
//=============================================================================
QJSValue MandalaApp::jsexec(QString s)
{
    QJSValue result;
    result = js->evaluate(s);
    if (result.isError()) {
        qWarning("%s", result.toString().toUtf8().data());
    }
    return result;
}
//=============================================================================
//=============================================================================
void MandalaApp::add_js(QString fname, QString description, QString body)
{
    jsexec(QString("function %1 { %2;};").arg(fname).arg(body));
    js_descr[fname] = description;
}
//=============================================================================
void MandalaApp::help()
{
    QString s;
    s += "<html><table width=100%>";
    foreach (const QString &cmd, js_descr.keys()) {
        s += "<tr><td valign='middle' style='background-color: #111; font-family: Monospace;'>";
        s += "<NOBR>&nbsp;<font color=cyan>";
        s += cmd;
        s += "</font></NOBR></td><td width='100%'>";
        s += "<font color=gray> &nbsp;" + js_descr.value(cmd) + "</font>";
        s += "</td></tr>";
    }
    s += "</table></font>";
    qDebug("%s", s.toUtf8().data());
}
//=============================================================================
void MandalaApp::serial(uint8_t portID, QJSValue data)
{
    /*bool bOK=false;
  uint pID=0;
  QByteArray ba;
  while(portID.isNumber()){
    pID=portID.toInteger();
    if(pID>255)break;
    ba=scrToArray(data);
    bOK=ba.size();
    break;
  }
  if(!bOK){
    qWarning("%s: %s, %s",tr("Can't parse command parameters").toUtf8().data(),portID.toString().toUtf8().data(),data.toString().toUtf8().data());
    return;
  }
  send_serial(pID,ba);*/
}
//=============================================================================
void MandalaApp::can(uint32_t can_ID, uint8_t can_IDE, QJSValue data)
{
    /*bool bOK=false;
  uint vID=0;
  uint vIDE=0;
  QByteArray ba;
  while(can_ID.isNumber()&&can_IDE.isNumber()){
    vID=can_ID.toInteger();
    vIDE=can_IDE.toInteger();
    if(vIDE>1)break;
    ba=scrToArray(data);
    bOK=ba.size();
    break;
  }
  if(!bOK){
    qWarning("%s: %s, %s, %s",tr("Can't parse command parameters").toUtf8().data(),can_ID.toString().toUtf8().data(),can_IDE.toString().toUtf8().data(),data.toString().toUtf8().data());
    return;
  }
  send_can(vID,vIDE,ba);*/
}
//=============================================================================
void MandalaApp::vmexec(QString func)
{
    /*bool bOK=false;
  QString s=func.toString();
  bOK=s.startsWith('@');
  if(!bOK){
    qWarning("%s: %s",tr("Can't parse command parameters").toUtf8().data(),func.toString().toUtf8().data());
    return;
  }
  send_vmexec(s);*/
}
//=============================================================================
void MandalaApp::sleep(uint ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, SLOT(quit()));
    loop.exec();
}
//=============================================================================
QString MandalaApp::latToString(double v)
{
    double lat = fabs(v);
    double lat_m = 60 * (lat - floor(lat)), lat_s = 60 * (lat_m - floor(lat_m)),
           lat_ss = 100 * (lat_s - floor(lat_s));
    return QString().sprintf("%c %g%c%02g'%02g.%02g\"",
                             (v >= 0) ? 'N' : 'S',
                             floor(lat),
                             176,
                             floor(lat_m),
                             floor(lat_s),
                             floor(lat_ss));
}
QString MandalaApp::lonToString(double v)
{
    double lat = fabs(v);
    double lat_m = 60 * (lat - floor(lat)), lat_s = 60 * (lat_m - floor(lat_m)),
           lat_ss = 100 * (lat_s - floor(lat_s));
    return QString().sprintf("%c %g%c%02g'%02g.%02g\"",
                             (v >= 0) ? 'E' : 'W',
                             floor(lat),
                             176,
                             floor(lat_m),
                             floor(lat_s),
                             floor(lat_ss));
}
double MandalaApp::latFromString(QString s)
{
    bool ok;
    int i;
    s = s.simplified();
    if (QString("NS").contains(s.at(0))) {
        bool bN = s.at(0) == 'N';
        s = s.remove(0, 1).trimmed();
        i = s.indexOf(QChar(176));
        double deg = s.left(i).toDouble(&ok);
        if (!ok)
            return 0;
        s = s.remove(0, i + 1).trimmed();
        i = s.indexOf('\'');
        double min = s.left(i).toDouble(&ok);
        if (!ok)
            return 0;
        s = s.remove(0, i + 1).trimmed();
        i = s.indexOf('\"');
        double sec = s.left(i).toDouble(&ok);
        if (!ok)
            return 0;
        deg = deg + min / 60.0 + sec / 3600.0;
        return bN ? deg : -deg;
    }
    return s.toDouble();
}
double MandalaApp::lonFromString(QString s)
{
    s = s.simplified();
    if (QString("EW").contains(s.at(0)))
        s[0] = (s.at(0) == 'E') ? 'N' : 'S';
    return latFromString(s);
}
QString MandalaApp::distanceToString(uint v)
{
    if (v >= 1000000)
        return QString("%1km").arg(v / 1000.0, 0, 'f', 0);
    if (v >= 1000)
        return QString("%1km").arg(v / 1000.0, 0, 'f', 1);
    return QString("%1m").arg((uint) v);
}
QString MandalaApp::timeToString(uint v)
{
    if (v == 0)
        return "--:--";
    qint64 d = (qint64) v / (24 * 60 * 60);
    if (d <= 0)
        return QString("%1").arg(QTime(0, 0, 0).addSecs(v).toString("hh:mm"));
    return QString("%1d%2").arg(d).arg(QTime(0, 0, 0).addSecs(v).toString("hh:mm"));
}
uint MandalaApp::timeFromString(QString s)
{
    uint t = 0;
    s = s.trimmed().toLower();
    if (s.contains('d')) {
        QString ds = s.left(s.indexOf('d')).trimmed();
        s = s.remove(0, s.indexOf('d') + 1).trimmed();
        bool ok = false;
        double dv = ds.toDouble(&ok);
        if (ok && dv > 0)
            t += floor(dv * (double) (24 * 60 * 60));
    }
    if (s.contains('h')) {
        QString ds = s.left(s.indexOf('h')).trimmed();
        s = s.remove(0, s.indexOf('h') + 1).trimmed();
        bool ok = false;
        double dv = ds.toDouble(&ok);
        if (ok && dv > 0)
            t += floor(dv * (double) (60 * 60));
    }
    if (s.contains('m')) {
        QString ds = s.left(s.indexOf('m')).trimmed();
        s = s.remove(0, s.indexOf('m') + 1).trimmed();
        bool ok = false;
        double dv = ds.toDouble(&ok);
        if (ok && dv > 0)
            t += floor(dv * (double) (60));
    }
    if (s.contains('s')) {
        QString ds = s.left(s.indexOf('s')).trimmed();
        s = s.remove(0, s.indexOf('s') + 1).trimmed();
        bool ok = false;
        double dv = ds.toDouble(&ok);
        if (ok && dv > 0)
            t += floor(dv);
        s.clear();
    }
    if (s.contains(':')) {
        QString ds = s.left(s.indexOf(':')).trimmed();
        s = s.remove(0, s.indexOf(':') + 1).trimmed();
        bool ok = false;
        double dv = ds.toDouble(&ok);
        if (ok && dv > 0)
            t += floor(dv * (double) (60 * 60));
        if (s.contains(':')) {
            QString ds = s.left(s.indexOf(':')).trimmed();
            s = s.remove(0, s.indexOf(':') + 1).trimmed();
            bool ok = false;
            double dv = ds.toDouble(&ok);
            if (ok && dv > 0)
                t += floor(dv * (double) (60));
        } else {
            bool ok = false;
            double dv = s.toDouble(&ok);
            if (ok && dv > 0)
                t += floor(dv * (double) (60));
            s.clear();
        }
    }
    if (!s.isEmpty()) {
        bool ok = false;
        double dv = s.toDouble(&ok);
        if (ok && dv > 0)
            t += floor(dv);
    }
    return t;
}
void MandalaApp::toolTip(QString tooltip)
{
    qDebug(":: %s", tooltip.toUtf8().data());
}
double MandalaApp::limit(double v, double min, double max)
{
    if (v < min)
        return min;
    if (v > max)
        return max;
    return v;
}
double MandalaApp::angle(double v)
{
    return _mandala::boundAngle(v);
}
double MandalaApp::angle360(double v)
{
    return _mandala::boundAngle360(v);
}
double MandalaApp::angle90(double v)
{
    return _mandala::boundAngle(v, 90);
}
//=============================================================================
void MandalaApp::sound(QString text)
{
    emit playSoundEffect(text);
}
//=============================================================================
//=============================================================================
// PROPERTIES
//=============================================================================
MandalaTree *MandalaApp::current()
{
    return m_current;
}
void MandalaApp::setCurrent(MandalaTree *v)
{
    if (!childItems.contains(v))
        return;
    if (m_current_set == v)
        return;
    m_current_set = v;
    static_cast<MandalaTreeVehicle *>(m_current)->bind(v);

    qDebug("!%s: %s", tr("Vehicle").toUtf8().data(), m_current->name().toUtf8().data());
}
//=============================================================================
bool MandalaApp::online()
{
    return onlineTimer.isActive();
}
void MandalaApp::updateOnline(bool v)
{
    bool bV = online();
    if (v)
        onlineTimer.start();
    else
        onlineTimer.stop();
    //qDebug()<<this<<bV<<v;
    if ((!v) || bV != v) {
        emit onlineChanged(v);
        sound(v ? "connected" : "error");
        if (!v)
            setErrcnt(0);
    }
}
//=============================================================================
//=============================================================================
void MandalaApp::downlinkReceived(const QByteArray &ba)
{
    MandalaTree::downlinkReceived(ba);
    qDebug("rx: %s", ba.toHex().toUpper().data());
}
//=============================================================================
