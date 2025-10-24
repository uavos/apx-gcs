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

#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>

class Aircraft : public QObject
{
    Q_OBJECT

    Q_PROPERTY(uint32_t icaoAddress READ icaoAddress WRITE setIcaoAddress NOTIFY dataChanged)
    Q_PROPERTY(int32_t latitude READ latitude WRITE setLatitude NOTIFY dataChanged)
    Q_PROPERTY(int32_t longitude READ longitude WRITE setLongitude NOTIFY dataChanged)
    Q_PROPERTY(uint16_t heading READ heading WRITE setHeading NOTIFY dataChanged)
    Q_PROPERTY(int32_t altitude READ altitude WRITE setAltitude NOTIFY dataChanged)
    Q_PROPERTY(uint16_t squawk READ squawk WRITE setSquawk NOTIFY dataChanged)
    Q_PROPERTY(QString callsign READ callsign WRITE setCallsign NOTIFY dataChanged)

public:
    explicit Aircraft(QObject *parent = nullptr)
        : QObject(parent)
    {}

    uint32_t icaoAddress() const { return m_icaoAddress; }
    int32_t latitude() const { return m_lat; }
    int32_t longitude() const { return m_lon; }
    uint16_t heading() const { return m_heading; }
    int32_t altitude() const { return m_altitude; }
    uint16_t squawk() const { return m_squawk; }
    QString callsign() const { return m_callsign; }
    QDateTime lastUpdate() const { return m_lastUpdate; }

    void setIcaoAddress(uint32_t val) { m_icaoAddress = val; }
    void setLatitude(int32_t val) { m_lat = val; }
    void setLongitude(int32_t val) { m_lon = val; }
    void setHeading(uint16_t val) { m_heading = val; }
    void setAltitude(int32_t val) { m_altitude = val; }
    void setSquawk(uint16_t val) { m_squawk = val; }
    void setCallsign(const QString &val) { m_callsign = val; }

    void update()
    {
        m_lastUpdate = QDateTime::currentDateTimeUtc();
        emit dataChanged();
    }

signals:
    void dataChanged();

private:
    uint32_t m_icaoAddress = 0;
    int32_t m_lat = 0;
    int32_t m_lon = 0;
    uint16_t m_heading = 0;
    int32_t m_altitude = 0;
    uint16_t m_squawk = 0;
    QString m_callsign;

    QDateTime m_lastUpdate;
};
