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
#ifndef QMANDALA_H
#define QMANDALA_H
#include <QtNetwork>
#include "QMandalaItem.h"
//=============================================================================
class QMandala : public QObject
{
  Q_OBJECT
public:
  explicit QMandala();
  static QMandala *instance(){return _instance;}

  QMandalaItem *current;
  QMandalaItem *local;

  //----------------------------------
  // global information
  class Global
  {
  public:
    static bool devMode();      // development run (i.e. from user home folder)
    static QDir plugins();      // plugins
    static QDir userPlugins();  // user plugins
    static QDir res();          // bitmaps,sounds
    static QDir user();         // local user files (Documents)
    static QDir telemetry();    // saved flight data
    static QDir maps();         // maps and tilesets
    static QDir lang();         // translations
    static QDir missions();     // saved flight plans
    static QDir configs();      // user saved nodes config files
    static QDir nodes();        // nodes backups
    static QDir scripts();      // user saved scripts

  };
  static QString version;    //software version
  static QString branch;    //software branch

  //----------------------------------
  //extra math related methods
  Q_INVOKABLE static QString latToString(double v);
  Q_INVOKABLE static QString lonToString(double v);
  Q_INVOKABLE double static latFromString(QString s);
  Q_INVOKABLE double static lonFromString(QString s);
  Q_INVOKABLE static QString distanceToString(uint v);
  Q_INVOKABLE static QString timeToString(uint v);
  Q_INVOKABLE uint static timeFromString(QString s);

  Q_INVOKABLE static void toolTip(QString tooltip);
  Q_INVOKABLE static double limit(double v,double min,double max);
  Q_INVOKABLE static double angle360(double v);
  Q_INVOKABLE static double angle90(double v);
  Q_INVOKABLE static double angle(double v);


  static QString vclassToString(IDENT::_vclass vclass);

  static MandalaCore::_vars_list vars_gcu;

  QList<QMandalaItem*>items;
private:
  static QMandala *_instance;
  QMandalaItem *append(void);
  QTimer identReqTimer;
  QTimer dlinkReqTimer;
  QList<QByteArray> req_list;
  void reqIDENT(IDENT::_squawk squawk);
  void jsexec(QString scr);
  bool checkSquawk(QMandalaItem *m_current,QMandalaItem *m,bool silent=false);
  void assignIDENT(IDENT::_ident *ident);

  QMandalaItem *prevUAV;
public slots:
  Q_INVOKABLE void setCurrent(QMandalaItem *m);
  Q_INVOKABLE void setCurrent(QString callsign);
  Q_INVOKABLE void setCurrentGCU(void);

  Q_INVOKABLE void changeCurrent(void);
  Q_INVOKABLE void changeCurrentNext(void);
  Q_INVOKABLE void changeCurrentPrev(void);

  void testUAV();

  Q_INVOKABLE void sound(QString text);

signals:
  void uavAdded(QMandalaItem *m);
  void uavRemoved(QMandalaItem *m);
  void currentChanged(QMandalaItem *m);

  void playSoundEffect(QString v);

  //Datalink
signals:
  void sendUplink(const QByteArray &ba);
public slots:
  void downlinkReceived(const QByteArray &ba);
  void upCntInc();
  void dlCntInc();
private slots:
  void uavSendUplink(const QByteArray &ba);
  void dlinkReqTimeout(void);
  void identReqTimeout(void);

  //wrappers
signals:
  void serverDiscovered(const QHostAddress address,const QString name);
  void connectToServer(const QHostAddress address);

  //PROPERTIES
public:
  Q_PROPERTY(bool jsValid READ jsValid WRITE setJsValid NOTIFY jsChanged)
  bool jsValid();
  Q_PROPERTY(bool online READ online WRITE setOnline NOTIFY onlineChanged)
  bool online();
  Q_PROPERTY(bool xpdrData READ xpdrData NOTIFY xpdrDataChanged)
  bool xpdrData();
  Q_PROPERTY(bool dlinkData READ dlinkData NOTIFY dlinkDataChanged)
  bool dlinkData();
  Q_PROPERTY(bool replayData READ replayData NOTIFY replayDataChanged)
  bool replayData();
  Q_PROPERTY(uint dlcnt READ dlcnt WRITE setDlcnt NOTIFY dlcntChanged)
  uint dlcnt();
  Q_PROPERTY(uint errcnt READ errcnt WRITE setErrcnt NOTIFY errcntChanged)
  uint errcnt();
  Q_PROPERTY(uint upcnt READ upcnt WRITE setUpcnt NOTIFY upcntChanged)
  uint upcnt();
  Q_PROPERTY(bool soundsEnabled READ soundsEnabled WRITE setsoundsEnabled NOTIFY soundsEnabledChanged)
  bool soundsEnabled();
  Q_PROPERTY(bool readOnly READ readOnly WRITE setReadOnly NOTIFY readOnlyChanged)
  bool readOnly();
  Q_PROPERTY(bool smooth READ smooth WRITE setSmooth NOTIFY smoothChanged)
  bool smooth();
  Q_PROPERTY(bool test READ test WRITE setTest NOTIFY testChanged)
  bool test();
  Q_PROPERTY(QString uavName READ uavName WRITE setUavName NOTIFY uavNameChanged)
  QString uavName();
  Q_PROPERTY(bool isLocal READ isLocal NOTIFY isLocalChanged)
  bool isLocal();
  Q_PROPERTY(uint size READ size WRITE setSize NOTIFY sizeChanged)
  uint size();
  Q_PROPERTY(QStringList uavNames READ uavNames NOTIFY sizeChanged)
  QStringList uavNames();
  Q_PROPERTY(bool isGCU READ isGCU NOTIFY uavNameChanged)
  bool isGCU();
  Q_PROPERTY(QMandalaItem *current READ mcurrent NOTIFY currentChanged)
  QMandalaItem * mcurrent();
private:
  bool m_jsValid;
  bool m_online;
  uint m_dlcnt;
  uint m_upcnt;
  bool m_soundsEnabled;
  bool m_readOnly;
  bool m_smooth;
  bool m_test;
  QString m_uavName;
  uint m_size;
signals:
  void jsChanged(bool);
  void onlineChanged(bool);
  void dlinkDataChanged(bool);
  void xpdrDataChanged(bool);
  void replayDataChanged(bool);
  void dlcntChanged(uint);
  void errcntChanged(uint);
  void upcntChanged(uint);
  void soundsEnabledChanged(bool);
  void smoothChanged(bool);
  void readOnlyChanged(bool);
  void testChanged(bool);
  void uavNameChanged(QString);
  void isLocalChanged(bool);
  void sizeChanged(uint);
public slots:
  Q_INVOKABLE void setJsValid(bool v);
  Q_INVOKABLE void setOnline(bool v);
  Q_INVOKABLE void setDlcnt(uint v);
  Q_INVOKABLE void setErrcnt(uint v);
  Q_INVOKABLE void setUpcnt(uint v);
  Q_INVOKABLE void setsoundsEnabled(bool v);
  Q_INVOKABLE void setSmooth(bool v);
  Q_INVOKABLE void setReadOnly(bool v);
  Q_INVOKABLE void setTest(bool v);
  Q_INVOKABLE void setUavName(QString v);
  Q_INVOKABLE void setSize(uint v);
private:
  QTimer onlineTimer;
private slots:
  void onlineTimeout();
};
//=============================================================================
#endif
