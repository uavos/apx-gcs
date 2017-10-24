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

  static QString version;    //software version
  static QString branch;    //software branch

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
  Q_PROPERTY(uint errcnt READ errcnt WRITE setErrcnt NOTIFY errcntChanged)
  uint errcnt();
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
  QString m_uavName;
  uint m_size;
signals:
  void jsChanged(bool);
  void onlineChanged(bool);
  void dlinkDataChanged(bool);
  void xpdrDataChanged(bool);
  void replayDataChanged(bool);
  void errcntChanged(uint);
  void uavNameChanged(QString);
  void isLocalChanged(bool);
  void sizeChanged(uint);
public slots:
  Q_INVOKABLE void setJsValid(bool v);
  Q_INVOKABLE void setOnline(bool v);
  Q_INVOKABLE void setErrcnt(uint v);
  Q_INVOKABLE void setUavName(QString v);
  Q_INVOKABLE void setSize(uint v);
private:
  QTimer onlineTimer;
private slots:
  void onlineTimeout();
};
//=============================================================================
#endif
