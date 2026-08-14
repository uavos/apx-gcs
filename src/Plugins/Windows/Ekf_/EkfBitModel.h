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

#include <QAbstractListModel>
#include <QColor>
#include <QString>
#include <QVector>

// Describes a single bit field entry for display
struct EkfBitEntry
{
    int bit;          // bit index
    QString name;     // field name from struct
    QString onColor;  // color when bit is set (empty = default green)
};

class EkfBitModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        BitRole = Qt::UserRole + 1,
        NameRole,
        LabelRole,
        OnColorRole,
    };

    explicit EkfBitModel(QObject *parent = nullptr);

    void setEntries(const QVector<EkfBitEntry> &entries);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Factory helpers — build models from MandalaBundles.h definitions
    static EkfBitModel *createFilterControlStatusLo(QObject *parent = nullptr); // bits 0-31
    static EkfBitModel *createFilterControlStatusHi(QObject *parent = nullptr); // bits 32+
    static EkfBitModel *createFaultStatus(QObject *parent = nullptr);
    static EkfBitModel *createEventStatus(QObject *parent = nullptr);

private:
    QVector<EkfBitEntry> m_entries;
};
