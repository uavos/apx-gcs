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
#ifndef QMANDALAITEM_H
#define QMANDALAITEM_H
//-----------------------------------------------------------------------------
#include <QtCore>
#include <QtScript/QScriptEngine>
#include "Mandala.h"
#include "node.h"
#include "FlightDataFile.h"
#include "QMandalaField.h"
class QProgressBar;
class QMandala;
//=============================================================================
class QMandalaItem : public QObject, public Mandala
{
  Q_OBJECT
public:
  QMandalaItem(QObject *parent=0,bool bbox=false);
  ~QMandalaItem();

  IDENT::_ident ident;
  bool bbox;

  //----------------------------------
  // information
  QByteArray md5;     //MD5 of all variable IDs (mandala ID)
  QHash<QString,QVariant> apcfg;
  void apcfgUpdated(void);
  void print_report(FILE *stream);
  //----------------------------------
  // data access
  QList<QMandalaField *>fields;
  QHash<QString,uint> constants;              //enums
  QHash<uint,QStringList> enumnames;         //enums
  QStringList names;//names, ordered
  QMandalaField *field(uint varmsk);
  Q_INVOKABLE QMandalaField *field(const QString &name);
  QString enumName(uint8_t varidx, int value) const;
private:
  QHash<QString,QMandalaField *>fieldsByName;  //excl. enums
  QHash<uint,QMandalaField *>fieldsByVarmsk;   //excl. enums
  QMandalaField *field_void;
  void registerField(uint varmsk,QString name,QString descr,QString units);

  //QtScript support
private:
  void add_scr(QString name,QString description,QString body);
  QHash<QString,QString> scr_descr; //alias,descr
  QByteArray scrToArray(QScriptValue data);
public:
  QScriptEngine engine;
  Q_INVOKABLE void scr_help();
  Q_INVOKABLE void scr_serial(QScriptValue portID,QScriptValue data);
  Q_INVOKABLE void scr_can(QScriptValue can_ID,QScriptValue can_IDE,QScriptValue data);
  Q_INVOKABLE void scr_vmexec(QScriptValue func);
  Q_INVOKABLE void scr_sleep(uint ms);

public:
  //----------------------------------
  // nodes and service
  const char * node_sn(const QByteArray &sn);
  const char * node_name(const QByteArray &sn);
  QHash<QByteArray,_node_info*> node_info;//all devices ever found in session: sn,node
private:
  int req_nstat_idx;
  QTimer nstatTimer;
  bool isBroadcast(const QByteArray &sn) const;
private slots:
  void req_nstat_next(void);

public:
  //telemetry recorder
  FlightDataFile *rec;

  //Datalink
signals:
  void sendUplink(const QByteArray &ba);
public slots:
  void downlinkReceived(const QByteArray &ba);

public slots:

  void send(unsigned char var_idx,const QByteArray &data);
  void send(unsigned char var_idx);
  void send_srv(uint cmd, const QByteArray &sn, const QByteArray &data=QByteArray());
  void send_serial(uint portNo, const QByteArray &data);
  void send_can(uint can_ID,bool can_IDE,const QByteArray &data);
  void request(uint var_idx);

  Q_INVOKABLE void send_vmexec(const QString &func);
  Q_INVOKABLE void exec_script(const QString &script);

  void emitUpdated(uint var_idx=idx_downstream);

  Q_INVOKABLE void req_nodes(void);
  Q_INVOKABLE void req_nstat(void);
signals:
  void serviceRequest(const QByteArray &packet);   //service packet received from UAV
  void updated(uint var_idx);           //var updated and received from UAV
  void data(uint8_t id,QByteArray data);  //packet idx that can't be extracted received from UAV
  void serialData(uint portNo,const QByteArray &ba);
  void canData(uint can_ID,bool can_IDE,const QByteArray &ba);

  void apcfgChanged();

  //PROPERTIES
public:
  Q_PROPERTY(bool dlinkData READ dlinkData WRITE setDlinkData NOTIFY dlinkDataChanged)
  bool dlinkData();
  Q_PROPERTY(bool xpdrData READ xpdrData WRITE setXpdrData NOTIFY xpdrDataChanged)
  bool xpdrData();
  Q_PROPERTY(bool replayData READ replayData WRITE setReplayData NOTIFY replayDataChanged)
  bool replayData();
  Q_PROPERTY(bool alive READ alive NOTIFY aliveChanged)
  bool alive();
  Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
  bool active();
  Q_PROPERTY(FlightDataFile *recorder READ recorder NOTIFY activeChanged)
  FlightDataFile * recorder();
  Q_PROPERTY(QString alarm READ alarm WRITE setAlarm NOTIFY alarmChanged)
  QString alarm();
  Q_PROPERTY(QString warning READ warning WRITE setWarning NOTIFY warningChanged)
  QString warning();
private:
  bool m_dlinkData;
  bool m_xpdrData;
  bool m_replayData;
  QTime dataTime;
  bool m_active;
  QString m_alarm;
  QString m_warning;
signals:
  void dlinkDataChanged(bool);
  void xpdrDataChanged(bool);
  void replayDataChanged(bool);
  void aliveChanged(bool);
  void activeChanged(bool);
  void alarmChanged(QString);
  void warningChanged(QString);
public slots:
  Q_INVOKABLE void setDlinkData(bool v);
  Q_INVOKABLE void setXpdrData(bool v);
  Q_INVOKABLE void setReplayData(bool v);
  Q_INVOKABLE void setActive(bool v);
  Q_INVOKABLE void setAlarm(QString v);
  Q_INVOKABLE void setWarning(QString v);
private:
  QTimer dlinkDataTimer;
  QTimer xpdrDataTimer;
  QTimer replayDataTimer;
private slots:
  void dlinkDataTimeout();
  void xpdrDataTimeout();
  void replayDataTimeout();
};
//=============================================================================
#endif
