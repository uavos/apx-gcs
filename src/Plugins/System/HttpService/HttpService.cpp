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
#include "HttpService.h"
#include <App/App.h>
#include <App/AppDirs.h>

#include <App/AppGcs.h>
#include <Datalink/Datalink.h>
#include <Fleet/Fleet.h>

namespace {
namespace HttpMessage {

using HttpCode = uint16_t;

enum ContentType {

    ApplicationXml,
    TextHtml,
};

QHash<HttpCode, QString> httpCodesStrTable{
    {200, "HTTP/1.0 200 OK\r\n"},
    {404, "HTTP/1.0 404 Not Found\r\n"},
};

QHash<ContentType, QString> contentTypeStrTable{
    {ContentType::ApplicationXml, "Content-Type: application/xml; charset=\"utf-8\"\r\n\r\n"},
    {ContentType::TextHtml, "Content-Type: text/html; charset=\"utf-8\"\r\n\r\n"},
};

QString buildHttpHeader(HttpCode code)
{
    auto it = httpCodesStrTable.find(code);
    return it != httpCodesStrTable.end() ? *it : httpCodesStrTable[404];
}

QString buildContentType(ContentType c_type)
{
    auto it = contentTypeStrTable.find(c_type);
    return it != contentTypeStrTable.end() ? *it : contentTypeStrTable[ContentType::TextHtml];
}

QString httpServerHostInfo(const QString &localAddr, quint16 localPort)
{
    return QString("<b>GCS HTTP Server</b> (%1:%2)").arg(localAddr).arg(localPort);
}

QString buildPayloadWithInfo()
{
    QString payload{};
    QTextStream s_payload{&payload};
    s_payload << "<hr size=1>";
    s_payload << QString("<a href=%1>%1</a> - %2<br>").arg("/kml").arg("Google Earth KML");
    s_payload << QString("<a href=%1>%1</a> - %2<br>")
                     .arg("/datalink")
                     .arg("Datalink stream [uint16 packet size][CRC_16_IBM][packet data]");
    s_payload << QString("<a href=%1>%1</a> - %2<br>")
                     .arg("/mandala")
                     .arg("Mandala XML data and commands");
    s_payload << QString("<br>More info here: <a href=%1>%1</a>").arg("https://docs.uavos.com/");
    return payload;
}
QString buildGcsVersionPayload()
{
    return QString("<br>GCS Version: <a>%1</a><br>").arg(AppBase::version());
}
QString buildPageNotFoundPayload(QString &req)
{
    return QString("<br>No service for '%1'").arg(req);
}
} // namespace HttpMessage
} // namespace

HttpService::HttpService(QObject *parent)
    : QObject(parent)
{
    connect(Fleet::instance(), &Fleet::unitSelected, this, &HttpService::unitSelected);
    unitSelected(Fleet::instance()->current());

    connect(AppGcs::instance()->f_datalink->f_server,
            &DatalinkServer::httpRequest,
            this,
            &HttpService::httpRequest);
}
void HttpService::unitSelected(Unit *unit)
{
    c_gps_lat = unit->f_mandala->fact("est.pos.lat");
    c_gps_lon = unit->f_mandala->fact("est.pos.lon");
    c_gps_hmsl = unit->f_mandala->fact("est.pos.hmsl");
    c_bearing = unit->f_mandala->fact("est.pos.bearing");
    c_roll = unit->f_mandala->fact("est.att.roll");
    c_pitch = unit->f_mandala->fact("est.att.pitch");
}

void HttpService::httpRequest(QTextStream &stream, QString req, const QTcpSocket *tcp)
{
    if (req == "/") {
        stream << HttpMessage::buildHttpHeader(200);
        stream << HttpMessage::buildContentType(HttpMessage::ContentType::TextHtml);
        stream << HttpMessage::httpServerHostInfo(tcp->localAddress().toString(), tcp->localPort());
        stream << HttpMessage::buildGcsVersionPayload();
        stream << HttpMessage::buildPayloadWithInfo();

    } else if (req.startsWith("/kml")) {
        stream << HttpMessage::buildHttpHeader(200);
        stream << (req.contains(".dae") ? "application/xml dae"
                                        : "application/vnd.google-earth.kml+xml");
        stream << "; charset=\"utf-8\"\r\n\r\n";
        stream << reply_google(req.mid(4));

    } else if (req.startsWith("/mandala")) {
        stream << HttpMessage::buildHttpHeader(200);
        stream << HttpMessage::buildContentType(HttpMessage::ContentType::ApplicationXml);
        stream << reply_mandala(req.contains('?') ? req.mid(req.indexOf("?") + 1) : "");

    } else {
        stream << HttpMessage::buildHttpHeader(404);
        stream << HttpMessage::buildContentType(HttpMessage::ContentType::TextHtml);
        stream << HttpMessage::httpServerHostInfo(tcp->localAddress().toString(), tcp->localPort());
        stream << HttpMessage::buildPageNotFoundPayload(req);
        stream << HttpMessage::buildPayloadWithInfo();
    }
}

/*void HttpService::readClient()
 {
  QTcpSocket* socket = (QTcpSocket*)sender();
  if (socket->canReadLine()) {
    QStringList tokens = QString(socket->readLine()).split(QRegularExpression("[ \r\n][ \r\n]*"));
    if (tokens[0]=="GET") {
      QString req=QUrl::fromPercentEncoding(tokens[1].toUtf8());
      QTextStream os(socket);
      os.setAutoDetectUnicode(true);
      os << "HTTP/1.0 200 Ok\r\n";
      if(req.startsWith("/kml")){
        os << (req.contains(".dae")?"application/xml dae":"application/vnd.google-earth.kml+xml");
        os << "; charset=\"utf-8\"\r\n";
      }else os << "Content-Type: application/xml; charset=\"utf-8\"\r\n";
      os << "\r\n";
      os << reply(req) << "\n";
      socket->close();
      if (socket->state() == QTcpSocket::UnconnectedState)
        delete socket;
    }
  }
}

QString HttpService::reply(const QString &req)
{
  if(req.startsWith("/kml"))
    return reply_google(req.mid(4));
  //if(req.isEmpty()||req.startsWith("/mandala"))
    return reply_mandala(req.contains('?')?req.mid(req.indexOf("?")+1):"");
  return QString();
}*/

QString HttpService::reply_mandala(const QString &req)
{
    QString reply;
    QXmlStreamWriter xml(&reply);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("mandala");
    //xml.writeTextElement("name","UAVOS GCU Mandala");
    //xml.writeTextElement("version",FactSystem::version());
    //------------------------------
    bool doDescr = false;
    QStringList rlist = req.trimmed().split('&', Qt::SkipEmptyParts);

    // select mandala
    Mandala *mandala = Fleet::instance()->current()->f_mandala;

    // collect requested facts
    QList<MandalaFact *> facts;
    for (const auto &s : rlist) {
        MandalaFact *f = nullptr;
        if (s.startsWith("descr")) {
            doDescr = true;
        } else if (s.contains('=')) {
            QString sname = s.left(s.indexOf('='));
            if (sname == "scr") {
                App::instance()->jsexec(s.mid(sname.size() + 1));
            } else {
                App::instance()->jsexec(s);
                f = mandala->fact(sname);
            }
        } else
            f = mandala->fact(s);
        if (f)
            facts.append(f);
    }
    bool bAllFacts = false;
    if (facts.isEmpty()) {
        facts = mandala->valueFacts();
        bAllFacts = true;
    }

    //mandala->currents
    for (auto f : facts) {
        if (doDescr) {
            xml.writeStartElement(f->mpath());
            xml.writeAttribute("descr", f->title());
            xml.writeAttribute("uid", QString::number(f->uid()));
            xml.writeAttribute("uid_hex", QString::number(f->uid(), 16).toUpper());
            xml.writeCharacters(f->valueText());
            xml.writeEndElement();
            continue;
        }

        if (bAllFacts && !f->everReceived())
            continue;

        xml.writeTextElement(f->mpath(), f->valueText());
    }
    //------------------------------
    xml.writeEndElement(); //mandala
    xml.writeEndDocument();
    return reply;
}

QString HttpService::reply_google(const QString &req)
{
    if (req.isEmpty() || req == "/")
        return reply_kml();
    if (req == "/telemetry")
        return reply_telemetry();
    if (req == "/flightplan")
        return reply_flightplan();
    if (req == "/chase")
        return reply_chase();
    if (req == "/chase_upd")
        return reply_chase_upd();

    if (req.contains(".dae")) {
        QFile f(AppDirs::res().filePath("bitmaps" + req));
        if (f.open(QIODevice::ReadOnly | QIODevice::Text))
            return f.readAll();
        //apxMsgW()<<"Model not found: %s",req.toUtf8().data());
    }
    return QString();
}

QString HttpService::reply_kml()
{
    QString reply;
    QXmlStreamWriter xml(&reply);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("kml");
    xml.writeAttribute("xmlns", "http://www.opengis.net/kml/2.2");
    xml.writeStartElement("Document");
    xml.writeTextElement("name", "UAVOS GCU Data");
    //------------------------------
    //folder contents
    //------------------------------
    xml.writeStartElement("NetworkLink");
    xml.writeTextElement("name", "Flight path from telemetry file");
    xml.writeStartElement("Link");
    xml.writeTextElement("href", "/telemetry");
    xml.writeEndElement(); //Link
    xml.writeEndElement(); //NetworkLink
    //------------------------------
    xml.writeStartElement("NetworkLink");
    xml.writeTextElement("name", "Chase cam view");
    xml.writeTextElement("visibility", "0");
    xml.writeTextElement("flyToView", "1");
    xml.writeStartElement("Link");
    xml.writeTextElement("href", "/chase");
    xml.writeTextElement("refreshMode", "onInterval");
    xml.writeTextElement("refreshInterval", "1");
    xml.writeEndElement(); //Link
    xml.writeEndElement(); //NetworkLink

    //------------------------------
    xml.writeStartElement("NetworkLink");
    xml.writeTextElement("name", "Flight Plan");
    xml.writeTextElement("visibility", "0");
    xml.writeStartElement("Link");
    xml.writeTextElement("href", "/flightplan");
    xml.writeTextElement("refreshMode", "onInterval");
    xml.writeTextElement("refreshInterval", "5");
    xml.writeEndElement(); //Link
    xml.writeEndElement(); //NetworkLink
    //------------------------------
    xml.writeEndElement(); //Document
    xml.writeEndElement(); //kml
    xml.writeEndDocument();
    return reply;
}

QString HttpService::reply_telemetry()
{
    QString reply;
    /*QXmlStreamWriter xml(&reply);
  xml.setAutoFormatting(true);
  xml.writeStartDocument();
  xml.writeStartElement("kml");
  xml.writeAttribute("xmlns","http://www.opengis.net/kml/2.2");
  xml.writeAttribute("xmlns:gx","http://www.google.com/kml/ext/2.2");
  xml.writeStartElement("Document");

  xml.writeStartElement("Style");
  xml.writeAttribute("id","traceStyle");
  xml.writeStartElement("LineStyle");
  xml.writeTextElement("color","7Fff1010");
  xml.writeTextElement("width","4");
  xml.writeEndElement();//LineStyle
  xml.writeStartElement("PolyStyle");
  xml.writeTextElement("color","7F801010");
  xml.writeEndElement();//PolyStyle
  xml.writeEndElement();//Style
  //------------------------------
  // FLIGHT TRACE
  xml.writeStartElement("Placemark");
  xml.writeTextElement("name",QFileInfo(Fleet::instance()->current()->f_recorder->loadFile).baseName());
  xml.writeTextElement("styleUrl","#traceStyle");
  xml.writeTextElement("visibility","0");
  xml.writeTextElement("gx:balloonVisibility","0");
  xml.writeStartElement("LineString");
  xml.writeTextElement("tessellate","0");
  xml.writeTextElement("altitudeMode","absolute");

  //fill trace coordinates
  int igps_lat=Fleet::instance()->current()->f_mandala->names.indexOf("gps_lat");
  int igps_lon=Fleet::instance()->current()->f_mandala->names.indexOf("gps_lon");
  int igps_hmsl=Fleet::instance()->current()->f_mandala->names.indexOf("gps_hmsl");
  QStringList st;
  for (const auto &vlist: Fleet::instance()->current()->f_recorder->file.data){
    const double lat=vlist.at(igps_lat);
    const double lon=vlist.at(igps_lon);
    const double hmsl=vlist.at(igps_hmsl);
    st.append(QString().sprintf("%.6f,%.6f,%.1f",lon,lat,hmsl));
  }
  xml.writeTextElement("coordinates",st.join("\n"));
  xml.writeEndElement();//LineString
  xml.writeEndElement();//Placemark
  //------------------------------
  // GCU POS
  xml.writeStartElement("Placemark");
  xml.writeTextElement("name","GCU location");
  xml.writeTextElement("visibility","1");
  xml.writeTextElement("gx:balloonVisibility","0");
  xml.writeStartElement("Point");
  xml.writeTextElement("altitudeMode","relativeToGround");
  xml.writeTextElement("coordinates",QString().sprintf("%.6f,%.6f,%.1f",Fleet::instance()->current()->f_recorder->fileValue("gps_home_lon"),Fleet::instance()->current()->f_recorder->fileValue("gps_home_lat"),0.0));
  xml.writeEndElement();//Point
  xml.writeEndElement();//Placemark
  //------------------------------
  // UAV MODEL
  xml.writeStartElement("Placemark");
  xml.writeAttribute("id","modelUavPM");
  xml.writeTextElement("name","UAV model");
  xml.writeTextElement("visibility","1");
  xml.writeTextElement("gx:balloonVisibility","0");
  xml.writeStartElement("Model");
  xml.writeTextElement("altitudeMode","absolute");
  xml.writeStartElement("Location");
  xml.writeAttribute("id","uavLoc");
  xml.writeTextElement("longitude",QString().sprintf("%.6f",Fleet::instance()->current()->f_recorder->fileValue("gps_lon")));
  xml.writeTextElement("latitude",QString().sprintf("%.6f",Fleet::instance()->current()->f_recorder->fileValue("gps_lat")));
  xml.writeTextElement("altitude",QString().sprintf("%.1f",Fleet::instance()->current()->f_recorder->fileValue("gps_hmsl")));
  xml.writeEndElement();//Location
  xml.writeStartElement("Orientation");
  xml.writeAttribute("id","uavOri");
  xml.writeTextElement("heading",0);
  xml.writeTextElement("tilt",0);
  xml.writeTextElement("roll",0);
  xml.writeEndElement();//Orientation
  xml.writeStartElement("Link");
  xml.writeTextElement("href","/models/uav.dae");
  xml.writeEndElement();//Link
  xml.writeEndElement();//Model
  xml.writeEndElement();//Placemark

  //------------------------------
  // TOUR FLYTO
  xml.writeStartElement("gx:Tour");
  xml.writeTextElement("name","Flight tour");
  xml.writeStartElement("gx:Playlist");
  int iroll=Fleet::instance()->current()->f_mandala->names.indexOf("roll");
  int ipitch=Fleet::instance()->current()->f_mandala->names.indexOf("pitch");
  int iyaw=Fleet::instance()->current()->f_mandala->names.indexOf("yaw");
  uint time_s=0;
  int i=-1,cam_i=-1;
  for (auto time: Fleet::instance()->current()->f_recorder->file.time){
    i++;
    if(cam_i>=0)cam_i++;
    //every second
    if((time-time_s)<1000)continue;
    time_s=time;
    const UnitRecorder::ListDouble &vlist=Fleet::instance()->current()->f_recorder->file.data.at(i);
    //find camera
    if(cam_i>=0){  //delay camera 2sec
      UnitRecorder::ListDouble vlist_cam=Fleet::instance()->current()->f_recorder->file.data.at(cam_i);
      xml.writeStartElement("gx:FlyTo");
      xml.writeTextElement("gx:flyToMode","smooth");
      xml.writeTextElement("gx:duration","1");  //1sec
      xml.writeStartElement("Camera");
      xml.writeTextElement("longitude",QString().sprintf("%.6f",vlist_cam.at(igps_lon)));
      xml.writeTextElement("latitude",QString().sprintf("%.6f",vlist_cam.at(igps_lat)));
      xml.writeTextElement("altitude",QString().sprintf("%.1f",vlist_cam.at(igps_hmsl)));
      double crs=vlist_cam.at(iyaw);
      xml.writeTextElement("heading",QString().sprintf("%.0f",crs<0?crs+360.0:crs));
      xml.writeTextElement("tilt",QString().sprintf("%.0f",vlist_cam.at(ipitch)+90.0));
      xml.writeTextElement("roll",QString().sprintf("%.0f",-vlist_cam.at(iroll)));
      xml.writeTextElement("altitudeMode","absolute");
      xml.writeEndElement();//Camera
      xml.writeEndElement();//FlyTo
    }else if(time>=3000) cam_i=0;

    xml.writeStartElement("gx:AnimatedUpdate");
    xml.writeTextElement("gx:duration","1");    //1sec
    xml.writeStartElement("Update");
    xml.writeTextElement("targetHref","");
    xml.writeStartElement("Change");
    xml.writeStartElement("Location");
    xml.writeAttribute("targetId","uavLoc");
    xml.writeTextElement("longitude",QString().sprintf("%.6f",vlist.at(igps_lon)));
    xml.writeTextElement("latitude",QString().sprintf("%.6f",vlist.at(igps_lat)));
    xml.writeTextElement("altitude",QString().sprintf("%.1f",vlist.at(igps_hmsl)));
    xml.writeEndElement();//Location
    xml.writeStartElement("Orientation");
    xml.writeAttribute("targetId","uavOri");
    xml.writeTextElement("heading",QString().sprintf("%.0f",FactSystem::angle360(vlist.at(iyaw)+180.0)));
    xml.writeTextElement("tilt",QString().sprintf("%.0f",vlist.at(ipitch)));
    xml.writeTextElement("roll",QString().sprintf("%.0f",vlist.at(iroll)));
    xml.writeEndElement();//Orientation
    xml.writeEndElement();//Change
    xml.writeEndElement();//Update
    xml.writeEndElement();//AnimatedUpdate

    //xml.writeStartElement("gx:Wait");
    //xml.writeTextElement("gx:duration","1");
    //xml.writeEndElement();//Wait
  }
  xml.writeEndElement();//Playlist
  xml.writeEndElement();//Tour
  //------------------------------
  // TRACK TIMELINE
  xml.writeStartElement("Placemark");
  xml.writeTextElement("name","Flight track timeline");
  xml.writeTextElement("visibility","1");
  xml.writeTextElement("gx:balloonVisibility","0");
  xml.writeStartElement("Style");
  xml.writeStartElement("IconStyle");
  xml.writeStartElement("Icon");
  xml.writeEndElement();
  xml.writeEndElement();
  xml.writeStartElement("LabelStyle");
  xml.writeTextElement("scale","0");
  xml.writeEndElement();
  xml.writeEndElement();
  xml.writeStartElement("gx:Track");
  xml.writeTextElement("altitudeMode","absolute");
  xml.writeTextElement("gx:horizFov","10");
  QDateTime t=Fleet::instance()->current()->f_recorder->file.timestamp;
  time_s=0;
  i=-1;
  for (auto time: Fleet::instance()->current()->f_recorder->file.time){
    i++;
    //every second
    if((time-time_s)<1000)continue;
    time_s=time;
    const UnitRecorder::ListDouble &vlist=Fleet::instance()->current()->f_recorder->file.data.at(i);
    xml.writeTextElement("when",t.toString("yyyy-MM-ddTHH:mm:ssZ"));
    xml.writeTextElement("gx:coord",QString().sprintf("%.6f,%.6f,%.1f",vlist.at(igps_lon),vlist.at(igps_lat),vlist.at(igps_hmsl)));
    double crs=FactSystem::angle(vlist.at(iyaw)+180.0);
    xml.writeTextElement("gx:angles",QString().sprintf("%.1f,%.1f,%.1f",crs,vlist.at(ipitch),vlist.at(iroll)));
    t=t.addSecs(1);
  }
  xml.writeStartElement("Model");
  xml.writeStartElement("Link");
  xml.writeTextElement("href","/models/uav.dae");
  xml.writeEndElement();//Link
  xml.writeEndElement();//Model

  xml.writeEndElement();//Track
  xml.writeEndElement();//Placemark
  //------------------------------



  xml.writeEndElement();//Document
  xml.writeEndElement();//kml
  xml.writeEndDocument();*/
    return reply;
}

QString HttpService::reply_flightplan()
{
    QString reply;
    QXmlStreamWriter xml(&reply);
    xml.setAutoFormatting(false);
    xml.writeStartDocument();
    xml.writeStartElement("kml");
    xml.writeAttribute("xmlns", "http://www.opengis.net/kml/2.2");
    xml.writeStartElement("Document");

    xml.writeStartElement("Style");
    xml.writeAttribute("id", "traceStyle");
    xml.writeStartElement("LineStyle");
    xml.writeTextElement("color", "7F303010");
    xml.writeTextElement("width", "4");
    xml.writeEndElement(); //LineStyle
    xml.writeEndElement(); //Style
    //------------------------------
    // waypoints
    xml.writeStartElement("Folder");
    xml.writeTextElement("name", "Waypoints");
    /*for(uint i=0;i<Fleet::instance()->current()->f_mandala->wpcnt;i++){
    xml.writeStartElement("Placemark");
    xml.writeTextElement("name","W"+QString::number(i+1));
    xml.writeTextElement("visibility","1");
    xml.writeStartElement("Point");
    xml.writeTextElement("altitudeMode","absolute");
    xml.writeTextElement("coordinates",QString().sprintf("%.6f,%.6f,%.0f",Fleet::instance()->current()->f_mandala->fp.waypoints[i].LLA[1],Fleet::instance()->current()->f_mandala->fp.waypoints[i].LLA[0],Fleet::instance()->current()->f_mandala->home_pos[2]+Fleet::instance()->current()->f_mandala->fp.waypoints[i].LLA[2]));
    xml.writeEndElement();//Point
    xml.writeEndElement();//Placemark
    }*/
    xml.writeEndElement(); //Folder

    //------------------------------
    //path
    xml.writeStartElement("Placemark");
    xml.writeTextElement("name", "Flight Plan path");
    xml.writeTextElement("styleUrl", "#traceStyle");
    xml.writeTextElement("gx:balloonVisibility", "0");
    xml.writeStartElement("LineString");
    xml.writeTextElement("tessellate", "0");
    xml.writeTextElement("altitudeMode", "absolute");
    QStringList st;
    /*for(uint i=0;i<Fleet::instance()->current()->f_mandala->wpcnt;i++)
    st.append(QString().sprintf("%.6f,%.6f,%.1f",Fleet::instance()->current()->f_mandala->fp.waypoints[i].LLA[1],Fleet::instance()->current()->f_mandala->fp.waypoints[i].LLA[0],Fleet::instance()->current()->f_mandala->home_pos[2]+Fleet::instance()->current()->f_mandala->fp.waypoints[i].LLA[2]));
  xml.writeTextElement("coordinates",st.join("\n"));*/
    xml.writeEndElement(); //LineString
    xml.writeEndElement(); //Placemark
    //------------------------------
    xml.writeEndElement(); //Document
    xml.writeEndElement(); //kml
    xml.writeEndDocument();
    return reply;
}

QString HttpService::reply_chase()
{
    QString reply;
    QXmlStreamWriter xml(&reply);
    xml.setAutoFormatting(false);
    xml.writeStartDocument();
    xml.writeStartElement("kml");
    xml.writeAttribute("xmlns", "http://www.opengis.net/kml/2.2");
    xml.writeStartElement("Document");
    //------------------------------
    //------------------------------
    //xml.writeStartElement("gx:Tour");
    //xml.writeTextElement("name","cam tour");
    //xml.writeStartElement("gx:Playlist");
    //xml.writeAttribute("id","camPlayList");

    //xml.writeStartElement("gx:FlyTo");
    //xml.writeTextElement("gx:flyToMode","smooth");
    //xml.writeTextElement("gx:duration","5.0");

    xml.writeStartElement("Camera");
    xml.writeAttribute("id", "camChase");
    xml.writeTextElement("longitude", c_gps_lon->valueText());
    xml.writeTextElement("latitude", c_gps_lat->valueText());
    xml.writeTextElement("altitude", c_gps_hmsl->valueText());
    xml.writeTextElement("heading",
                         QString("%1").arg(AppRoot::angle360(c_bearing->value().toDouble()),
                                           0,
                                           'f'));
    xml.writeTextElement("tilt", QString("%1").arg(c_pitch->value().toDouble() + 90.0));
    xml.writeTextElement("roll", QString("%1").arg(-c_roll->value().toDouble()));
    xml.writeTextElement("altitudeMode", "absolute");
    xml.writeEndElement(); //Camera
    //xml.writeEndElement();//FlyTo

    //xml.writeEndElement();//Playlist
    //xml.writeEndElement();//Tour
    //------------------------------
    //------------------------------
    xml.writeEndElement(); //Document
    xml.writeEndElement(); //kml
    xml.writeEndDocument();
    return reply;
}
QString HttpService::reply_chase_upd()
{
    QString reply;
    QXmlStreamWriter xml(&reply);
    xml.setAutoFormatting(false);
    xml.writeStartDocument();
    xml.writeStartElement("kml");
    xml.writeAttribute("xmlns", "http://www.opengis.net/kml/2.2");
    //------------------------------
    xml.writeStartElement("NetworkLinkControl");
    //xml.writeStartElement("gx:AnimatedUpdate");
    //xml.writeTextElement("gx:duration","2.0");
    xml.writeStartElement("Update");
    xml.writeTextElement("targetHref", "/chase");
    xml.writeStartElement("Create");

    xml.writeStartElement("gx:Playlist");
    xml.writeAttribute("id", "camPlayList");
    xml.writeStartElement("gx:FlyTo");
    xml.writeTextElement("gx:flyToMode", "smooth");
    xml.writeTextElement("gx:duration", "2.0");
    xml.writeStartElement("Camera");
    xml.writeAttribute("id", "camChase");
    xml.writeTextElement("longitude", c_gps_lon->valueText());
    xml.writeTextElement("latitude", c_gps_lat->valueText());
    xml.writeTextElement("altitude", c_gps_hmsl->valueText());
    xml.writeTextElement("heading",
                         QString("%1").arg(AppRoot::angle360(c_bearing->value().toDouble()),
                                           0,
                                           'f'));
    xml.writeTextElement("tilt", QString("%1").arg(c_pitch->value().toDouble() + 90.0));
    xml.writeTextElement("roll", QString("%1").arg(-c_roll->value().toDouble()));
    xml.writeTextElement("altitudeMode", "absolute");
    xml.writeEndElement(); //Camera
    xml.writeEndElement(); //FlyTo
    xml.writeEndElement(); //Playlist

    xml.writeEndElement(); //Create
    xml.writeEndElement(); //Update
    //xml.writeEndElement();//AnimatedUpdate
    xml.writeEndElement(); //NetworkLinkControl
    //------------------------------
    xml.writeEndElement(); //kml
    xml.writeEndDocument();
    return reply;
}
