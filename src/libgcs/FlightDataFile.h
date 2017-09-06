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
#ifndef FLIGHTDATAFILE_H
#define FLIGHTDATAFILE_H
#include <QtCore>
#include "Mandala.h"
class QMandala;
class QMandalaItem;
class QMandalaField;
class QProgressBar;
//=============================================================================
class FlightDataFile : public QObject
{
  Q_OBJECT
public:
  FlightDataFile(QMandalaItem *parent);

  QString uavNameOverride;

  uint recFlightNo;
  bool recDisable;  //externally disable recording (f.ex. by player)

  const QStringList recFileNames(void);

  void saveXmlPart(QString key,QByteArray data); //save and record when started, or immediate
  void record_xml(const QByteArray &data); //record immediate if recording

  //reader
  typedef QList<double> ListDouble;
  typedef QMultiHash<uint,QString> XmlPart;//xmlParts (nodes,mission..) on time_ms
  struct{
    QDateTime timestamp;
    QList<uint> time;               // time_ms per index
    QList<ListDouble> data;         // src_vars as float
    QHash<uint,QStringList> msg;    // text messages on time_ms
    QHash<QString,QString> params;  // Name: Value pairs from file
    QHash<QString,XmlPart> xmlParts; //xmlParts by name
    QString notes;
  }file;

  QFile loadFile;

  bool loadFlight(int idx,QProgressBar *progressBar=NULL);
  bool loadFlight(const QString &fName,QProgressBar *progressBar=NULL);
  double value(const QString &name, int pos=0);

private:
  QMandala *mandala;
  QMandalaItem *mvar;
  Mandala m;
  QString recFileSuffix;

  QMap<QString,QByteArray> saveXmlParts;

  //writer
  QFile recFile;
  QXmlStreamWriter xmlWriter;
  bool header_written;
  ListDouble streamList;
  QString fieldToString(QMandalaField *f,double v);

  bool recTrigger;
  QTimer monitorTimer,recStopTimer;
  QDateTime recStartTime;

  void loadFromText(QProgressBar *progressBar);
  void loadFromXml(QProgressBar *progressBar);

  QString readXmlPart(QXmlStreamReader &xml);

  //nodes save
  bool is_apc_conf(const QByteArray &ba);

  //recorder
  bool record_check(void);
  void record_header(void);
  void record_data(QString tag, const QByteArray &data);
private slots:
  void monitorTimerTimeout(void);
public slots:
  void flush();
  void stop_recording();

  //void convertFormat(QString fileName=QString()); //find and convert old format files

  void record_downlink(const QByteArray &data);
  void record_uplink(const QByteArray &data);

signals:
  void fileLoaded();
  void replay_progress(uint time_ms); //forward signal from telemetry player

  //PROPERTIES
public:
  Q_PROPERTY(bool recording READ recording WRITE setRecording NOTIFY recordingChanged)
  bool recording();
  Q_PROPERTY(uint size READ size NOTIFY sizeChanged)
  uint size();
  Q_PROPERTY(QTime time READ time NOTIFY sizeChanged)
  QTime time();
private:
  bool m_recording;
  uint m_size;
  void setSize(uint v);
signals:
  void recordingChanged(bool);
  void sizeChanged(uint);
  void timeChanged(uint);
public slots:
  Q_INVOKABLE void setRecording(bool v);
  Q_INVOKABLE void close(void);
  Q_INVOKABLE void discard(void);
};
//=============================================================================
#endif // FLIGHTDATAFILE_H
