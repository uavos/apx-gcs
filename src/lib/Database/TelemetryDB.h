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
#ifndef TelemetryDB_H
#define TelemetryDB_H
#include <FactSystem.h>
#include "DatabaseConnection.h"
class Vehicle;
//=============================================================================
class TelemetryDB : public DatabaseConnection
{
  Q_OBJECT
public:
  explicit TelemetryDB(QObject *parent, QString sessionName, Vehicle *vehicle=NULL, bool readOnly=false);

  QHash<Fact*,quint64> recFacts;


  quint64 writeRecord(const QString &vehicleUID, const QString &callsign, const QString &comment, bool rec, quint64 timestamp);
  bool writeDownlink(quint64 telemetryID, quint64 timestamp, const QList<Fact*> &facts);
  bool writeField(quint64 telemetryID, quint64 timestamp, quint64 fieldID, const QVariant &v, bool uplink);
  bool writeField(QSqlQuery &query, quint64 telemetryID, quint64 timestamp, quint64 fieldID, const QVariant &v, bool uplink);
  bool writeEvent(quint64 telemetryID, quint64 timestamp, const QString &name, const QString &value, bool uplink=false, const QByteArray &data=QByteArray());

  bool writeNotes(quint64 telemetryID, const QString &s);

  bool deleteRecord(quint64 telemetryID);


  bool createTelemetryTable(quint64 telemetryID); //for player to temp table
  bool readDownlink(quint64 telemetryID, quint64 time); //and update facts
};
//=============================================================================
#endif // FLIGHTDATAFILE_H
