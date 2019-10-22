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
#include "HttpService.h"
#include <App/App.h>
#include <App/AppDirs.h>
#include <Vehicles/Vehicles.h>
//=============================================================================
HttpService::HttpService(QObject *parent)
    : QObject(parent)
    , c_gps_lat("gps_lat")
    , c_gps_lon("gps_lon")
    , c_gps_hmsl("gps_hmsl")
    , c_course("course")
    , c_roll("roll")
    , c_pitch("pitch")
{}
//=============================================================================
void HttpService::httpRequest(QTextStream &stream, QString req, bool *ok)
{
    if (req.startsWith("/kml")) {
        stream << "HTTP/1.0 200 Ok\r\n";
        stream << (req.contains(".dae") ? "application/xml dae"
                                        : "application/vnd.google-earth.kml+xml");
        stream << "; charset=\"utf-8\"\r\n";
        stream << "\r\n";
        stream << reply_google(req.mid(4));
        *ok = true;
        return;
    }
    if (req.startsWith("/mandala")) {
        stream << "HTTP/1.0 200 Ok\r\n";
        stream << "Content-Type: application/xml; charset=\"utf-8\"\r\n";
        stream << "\r\n";
        stream << reply_mandala(req.contains('?') ? req.mid(req.indexOf("?") + 1) : "");
        *ok = true;
        return;
    }
}
//=============================================================================
//=============================================================================
/*void HttpService::readClient()
 {
  QTcpSocket* socket = (QTcpSocket*)sender();
  if (socket->canReadLine()) {
    QStringList tokens = QString(socket->readLine()).split(QRegExp("[ \r\n][ \r\n]*"));
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
//=============================================================================
QString HttpService::reply(const QString &req)
{
  if(req.startsWith("/kml"))
    return reply_google(req.mid(4));
  //if(req.isEmpty()||req.startsWith("/mandala"))
    return reply_mandala(req.contains('?')?req.mid(req.indexOf("?")+1):"");
  return QString();
}*/
//=============================================================================
//=============================================================================
//=============================================================================
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
    QStringList sv = req.split('&', QString::SkipEmptyParts);
    if (sv.isEmpty())
        sv = Vehicles::instance()->current()->f_mandala->names;
    else {
        QStringList st;
        foreach (QString s, sv) {
            if (s.startsWith("descr")) {
                doDescr = true;
                s = "";
            } else if (s.contains('=')) {
                QString sname = s.left(s.indexOf('='));
                if (sname == "scr") {
                    App::instance()->jsexec(s.mid(sname.size() + 1));
                    s = "";
                } else {
                    App::instance()->jsexec(
                        s); //QString("set('%1',%2)").arg(sname).arg(s.mid(sname.size()+1)));
                    s = sname;
                }
            }
            if (s.size())
                st.append(s);
        }
        if (st.size())
            sv = st;
        else
            sv = Vehicles::instance()->current()->f_mandala->names;
    }
    //mandala->currents
    foreach (QString vname, sv) {
        QString s = QString("%1").arg(
            Vehicles::instance()->current()->f_mandala->valueByName(vname).toDouble(), 0, 'f', 10);
        while (s.at(s.size() - 1) == '0') { //remove trailing zeros
            s.remove(s.size() - 1, 1);
            if (s.at(s.size() - 1) != '.')
                continue;
            s.remove(s.size() - 1, 1);
            break;
        }
        if (doDescr) {
            xml.writeStartElement(vname);
            xml.writeAttribute(
                "descr", Vehicles::instance()->current()->f_mandala->factByName(vname)->descr());
            uint idx = Vehicles::instance()->current()->f_mandala->factByName(vname)->id();
            xml.writeAttribute("id", QString::number(idx & 0xFF));
            xml.writeAttribute("id_hex", QString::number(idx & 0xFF, 16).toUpper());
            xml.writeAttribute("id_fact", QString::number(idx, 16).toUpper());
            xml.writeCharacters(s);
            xml.writeEndElement();
        } else {
            xml.writeTextElement(vname, s);
        }
    }
    //------------------------------
    xml.writeEndElement(); //mandala
    xml.writeEndDocument();
    return reply;
}
//=============================================================================
//=============================================================================
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
//=============================================================================
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
//=============================================================================
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
  xml.writeTextElement("name",QFileInfo(Vehicles::instance()->current()->f_recorder->loadFile).baseName());
  xml.writeTextElement("styleUrl","#traceStyle");
  xml.writeTextElement("visibility","0");
  xml.writeTextElement("gx:balloonVisibility","0");
  xml.writeStartElement("LineString");
  xml.writeTextElement("tessellate","0");
  xml.writeTextElement("altitudeMode","absolute");

  //fill trace coordinates
  int igps_lat=Vehicles::instance()->current()->f_mandala->names.indexOf("gps_lat");
  int igps_lon=Vehicles::instance()->current()->f_mandala->names.indexOf("gps_lon");
  int igps_hmsl=Vehicles::instance()->current()->f_mandala->names.indexOf("gps_hmsl");
  QStringList st;
  foreach(const VehicleRecorder::ListDouble &vlist,Vehicles::instance()->current()->f_recorder->file.data){
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
  xml.writeTextElement("coordinates",QString().sprintf("%.6f,%.6f,%.1f",Vehicles::instance()->current()->f_recorder->fileValue("gps_home_lon"),Vehicles::instance()->current()->f_recorder->fileValue("gps_home_lat"),0.0));
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
  xml.writeTextElement("longitude",QString().sprintf("%.6f",Vehicles::instance()->current()->f_recorder->fileValue("gps_lon")));
  xml.writeTextElement("latitude",QString().sprintf("%.6f",Vehicles::instance()->current()->f_recorder->fileValue("gps_lat")));
  xml.writeTextElement("altitude",QString().sprintf("%.1f",Vehicles::instance()->current()->f_recorder->fileValue("gps_hmsl")));
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
  int iroll=Vehicles::instance()->current()->f_mandala->names.indexOf("roll");
  int ipitch=Vehicles::instance()->current()->f_mandala->names.indexOf("pitch");
  int iyaw=Vehicles::instance()->current()->f_mandala->names.indexOf("yaw");
  uint time_s=0;
  int i=-1,cam_i=-1;
  foreach(uint time,Vehicles::instance()->current()->f_recorder->file.time){
    i++;
    if(cam_i>=0)cam_i++;
    //every second
    if((time-time_s)<1000)continue;
    time_s=time;
    const VehicleRecorder::ListDouble &vlist=Vehicles::instance()->current()->f_recorder->file.data.at(i);
    //find camera
    if(cam_i>=0){  //delay camera 2sec
      VehicleRecorder::ListDouble vlist_cam=Vehicles::instance()->current()->f_recorder->file.data.at(cam_i);
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
  QDateTime t=Vehicles::instance()->current()->f_recorder->file.timestamp;
  time_s=0;
  i=-1;
  foreach(uint time,Vehicles::instance()->current()->f_recorder->file.time){
    i++;
    //every second
    if((time-time_s)<1000)continue;
    time_s=time;
    const VehicleRecorder::ListDouble &vlist=Vehicles::instance()->current()->f_recorder->file.data.at(i);
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
//=============================================================================
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
    /*for(uint i=0;i<Vehicles::instance()->current()->f_mandala->wpcnt;i++){
    xml.writeStartElement("Placemark");
    xml.writeTextElement("name","W"+QString::number(i+1));
    xml.writeTextElement("visibility","1");
    xml.writeStartElement("Point");
    xml.writeTextElement("altitudeMode","absolute");
    xml.writeTextElement("coordinates",QString().sprintf("%.6f,%.6f,%.0f",Vehicles::instance()->current()->f_mandala->fp.waypoints[i].LLA[1],Vehicles::instance()->current()->f_mandala->fp.waypoints[i].LLA[0],Vehicles::instance()->current()->f_mandala->home_pos[2]+Vehicles::instance()->current()->f_mandala->fp.waypoints[i].LLA[2]));
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
    /*for(uint i=0;i<Vehicles::instance()->current()->f_mandala->wpcnt;i++)
    st.append(QString().sprintf("%.6f,%.6f,%.1f",Vehicles::instance()->current()->f_mandala->fp.waypoints[i].LLA[1],Vehicles::instance()->current()->f_mandala->fp.waypoints[i].LLA[0],Vehicles::instance()->current()->f_mandala->home_pos[2]+Vehicles::instance()->current()->f_mandala->fp.waypoints[i].LLA[2]));
  xml.writeTextElement("coordinates",st.join("\n"));*/
    xml.writeEndElement(); //LineString
    xml.writeEndElement(); //Placemark
    //------------------------------
    xml.writeEndElement(); //Document
    xml.writeEndElement(); //kml
    xml.writeEndDocument();
    return reply;
}
//=============================================================================
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
    xml.writeTextElement("longitude", QString().sprintf("%.6f", (double) c_gps_lon));
    xml.writeTextElement("latitude", QString().sprintf("%.6f", (double) c_gps_lat));
    xml.writeTextElement("altitude", QString().sprintf("%.1f", (double) c_gps_hmsl));
    xml.writeTextElement("heading",
                         QString().sprintf("%.0f",
                                           (double) c_course < 0 ? (double) c_course + 360.0
                                                                 : (double) c_course));
    xml.writeTextElement("tilt", QString().sprintf("%.0f", (double) c_pitch + 90.0));
    xml.writeTextElement("roll", QString().sprintf("%.0f", -(double) c_roll));
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
    xml.writeTextElement("longitude", QString().sprintf("%.6f", (double) c_gps_lon));
    xml.writeTextElement("latitude", QString().sprintf("%.6f", (double) c_gps_lat));
    xml.writeTextElement("altitude", QString().sprintf("%.1f", (double) c_gps_hmsl));
    xml.writeTextElement("heading",
                         QString().sprintf("%.0f",
                                           (double) c_course < 0 ? (double) c_course + 360.0
                                                                 : (double) c_course));
    xml.writeTextElement("tilt", QString().sprintf("%.0f", (double) c_pitch + 90.0));
    xml.writeTextElement("roll", QString().sprintf("%.0f", -(double) c_roll));
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
//=============================================================================
