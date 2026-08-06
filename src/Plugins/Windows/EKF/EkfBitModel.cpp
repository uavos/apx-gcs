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
#include "EkfBitModel.h"

#include <MandalaBundles.h>

// Compile-time check: verify field names match MandalaBundles.h 
// If a field is renamed/removed in the struct, this will fail to compile.

static constexpr void _check_filter_control_status_fields()
{
    mandala::bundle::filter_control_status_u u{};
    (void)u.flags.tilt_align;
    (void)u.flags.yaw_align;
    (void)u.flags.gnss_pos;
    (void)u.flags.opt_flow;
    (void)u.flags.mag_hdg;
    (void)u.flags.mag_3D;
    (void)u.flags.mag_dec;
    (void)u.flags.in_air;
    (void)u.flags.wind;
    (void)u.flags.baro_hgt;
    (void)u.flags.rng_hgt;
    (void)u.flags.gps_hgt;
    (void)u.flags.ev_pos;
    (void)u.flags.ev_yaw;
    (void)u.flags.ev_hgt;
    (void)u.flags.fuse_beta;
    (void)u.flags.mag_field_disturbed;
    (void)u.flags.fixed_wing;
    (void)u.flags.mag_fault;
    (void)u.flags.fuse_aspd;
    (void)u.flags.gnd_effect;
    (void)u.flags.rng_stuck;
    (void)u.flags.gnss_yaw;
    (void)u.flags.mag_aligned_in_flight;
    (void)u.flags.ev_vel;
    (void)u.flags.synthetic_mag_z;
    (void)u.flags.vehicle_at_rest;
    (void)u.flags.gnss_yaw_fault;
    (void)u.flags.rng_fault;
    (void)u.flags.inertial_dead_reckoning;
    (void)u.flags.wind_dead_reckoning;
    (void)u.flags.rng_kin_consistent;
    (void)u.flags.fake_pos;
    (void)u.flags.fake_hgt;
    (void)u.flags.gravity_vector;
    (void)u.flags.mag;
    (void)u.flags.ev_yaw_fault;
    (void)u.flags.mag_heading_consistent;
    (void)u.flags.aux_gpos;
    (void)u.flags.rng_terrain;
    (void)u.flags.opt_flow_terrain;
    (void)u.flags.valid_fake_pos;
    (void)u.flags.constant_pos;
    (void)u.flags.baro_fault;
    (void)u.flags.gnss_vel;
    (void)u.flags.gnss_fault;
    (void)u.flags.yaw_manual;
    (void)u.flags.gnss_hgt_fault;
    (void)u.flags.in_transition_to_fw;
}

static constexpr void _check_fault_status_fields()
{
    mandala::bundle::fault_status_u u{};
    (void)u.flags.bad_mag_x;
    (void)u.flags.bad_mag_y;
    (void)u.flags.bad_mag_z;
    (void)u.flags.bad_hdg;
    (void)u.flags.bad_mag_decl;
    (void)u.flags.bad_airspeed;
    (void)u.flags.bad_sideslip;
    (void)u.flags.bad_optflow_X;
    (void)u.flags.bad_optflow_Y;
    (void)u.flags.__UNUSED;
    (void)u.flags.bad_acc_vertical;
    (void)u.flags.bad_acc_clipping;
}

static constexpr void _check_event_status_fields()
{
    mandala::bundle::information_event_status_u u{};
    (void)u.flags.gps_checks_passed;
    (void)u.flags.reset_vel_to_gps;
    (void)u.flags.reset_vel_to_flow;
    (void)u.flags.reset_vel_to_vision;
    (void)u.flags.reset_vel_to_zero;
    (void)u.flags.reset_pos_to_last_known;
    (void)u.flags.reset_pos_to_gps;
    (void)u.flags.reset_pos_to_vision;
    (void)u.flags.starting_gps_fusion;
    (void)u.flags.starting_vision_pos_fusion;
    (void)u.flags.starting_vision_vel_fusion;
    (void)u.flags.starting_vision_yaw_fusion;
    (void)u.flags.yaw_aligned_to_imu_gps;
    (void)u.flags.reset_hgt_to_baro;
    (void)u.flags.reset_hgt_to_gps;
    (void)u.flags.reset_hgt_to_rng;
    (void)u.flags.reset_hgt_to_ev;
    (void)u.flags.reset_pos_to_ext_obs;
    (void)u.flags.reset_wind_to_ext_obs;
}

// Suppress unused-function warning — these are compile-time checks only
[[maybe_unused]] static constexpr auto _field_checks = [] {
    _check_filter_control_status_fields();
    _check_fault_status_fields();
    _check_event_status_fields();
    return 0;
}();

// Colors
static const QString colorRed = QStringLiteral("#cc2222");
static const QString colorYellow = QStringLiteral("#ffe600");
static const QString colorNone; // empty = default green in QML

// filter_control_status_u (bits 0-31)
static const QVector<EkfBitEntry> s_filterControlStatusLo = {
    { 0, QStringLiteral("tilt_align"),             colorNone},
    { 1, QStringLiteral("yaw_align"),              colorNone},
    { 2, QStringLiteral("gnss_pos"),               colorNone},
    { 3, QStringLiteral("opt_flow"),               colorNone},
    { 4, QStringLiteral("mag_hdg"),                colorNone},
    { 5, QStringLiteral("mag_3D"),                 colorNone},
    { 6, QStringLiteral("mag_dec"),                colorNone},
    { 7, QStringLiteral("in_air"),                 colorNone},
    { 8, QStringLiteral("wind"),                   colorNone},
    { 9, QStringLiteral("baro_hgt"),               colorNone},
    {10, QStringLiteral("rng_hgt"),                colorNone},
    {11, QStringLiteral("gps_hgt"),                colorNone},
    {12, QStringLiteral("ev_pos"),                 colorNone},
    {13, QStringLiteral("ev_yaw"),                 colorNone},
    {14, QStringLiteral("ev_hgt"),                 colorNone},
    {15, QStringLiteral("fuse_beta"),              colorNone},
    {16, QStringLiteral("mag_field_disturbed"),    colorRed},
    {17, QStringLiteral("fixed_wing"),             colorNone},
    {18, QStringLiteral("mag_fault"),              colorRed},
    {19, QStringLiteral("fuse_aspd"),              colorNone},
    {20, QStringLiteral("gnd_effect"),             colorYellow},
    {21, QStringLiteral("rng_stuck"),              colorRed},
    {22, QStringLiteral("gnss_yaw"),               colorNone},
    {23, QStringLiteral("mag_aligned_in_flight"),  colorNone},
    {24, QStringLiteral("ev_vel"),                 colorNone},
    {25, QStringLiteral("synthetic_mag_z"),        colorNone},
    {26, QStringLiteral("vehicle_at_rest"),        colorNone},
    {27, QStringLiteral("gnss_yaw_fault"),         colorRed},
    {28, QStringLiteral("rng_fault"),              colorRed},
    {29, QStringLiteral("inertial_dead_reckoning"), colorRed},
    {30, QStringLiteral("wind_dead_reckoning"),    colorYellow},
    {31, QStringLiteral("rng_kin_consistent"),     colorNone},
};

// filter_control_status_u (bits 32+)
static const QVector<EkfBitEntry> s_filterControlStatusHi = {
    {32, QStringLiteral("fake_pos"),               colorYellow},
    {33, QStringLiteral("fake_hgt"),               colorYellow},
    {34, QStringLiteral("gravity_vector"),         colorNone},
    {35, QStringLiteral("mag"),                    colorNone},
    {36, QStringLiteral("ev_yaw_fault"),           colorRed},
    {37, QStringLiteral("mag_heading_consistent"), colorNone},
    {38, QStringLiteral("aux_gpos"),               colorNone},
    {39, QStringLiteral("rng_terrain"),            colorNone},
    {40, QStringLiteral("opt_flow_terrain"),       colorNone},
    {41, QStringLiteral("valid_fake_pos"),         colorYellow},
    {42, QStringLiteral("constant_pos"),           colorNone},
    {43, QStringLiteral("baro_fault"),             colorRed},
    {44, QStringLiteral("gnss_vel"),               colorNone},
    {45, QStringLiteral("gnss_fault"),             colorRed},
    {46, QStringLiteral("yaw_manual"),             colorNone},
    {47, QStringLiteral("gnss_hgt_fault"),         colorRed},
    {48, QStringLiteral("in_transition_to_fw"),    colorNone},
};

// fault_status_u
static const QVector<EkfBitEntry> s_faultStatus = {
    { 0, QStringLiteral("bad_mag_x"),        colorRed},
    { 1, QStringLiteral("bad_mag_y"),        colorRed},
    { 2, QStringLiteral("bad_mag_z"),        colorRed},
    { 3, QStringLiteral("bad_hdg"),          colorRed},
    { 4, QStringLiteral("bad_mag_decl"),     colorRed},
    { 5, QStringLiteral("bad_airspeed"),     colorRed},
    { 6, QStringLiteral("bad_sideslip"),     colorRed},
    { 7, QStringLiteral("bad_optflow_X"),    colorRed},
    { 8, QStringLiteral("bad_optflow_Y"),    colorRed},
    { 9, QStringLiteral("(unused)"),         colorNone},
    {10, QStringLiteral("bad_acc_vertical"), colorRed},
    {11, QStringLiteral("bad_acc_clipping"), colorRed},
};

// information_event_status_u
static const QVector<EkfBitEntry> s_eventStatus = {
    { 0, QStringLiteral("gps_checks_passed"),          colorNone},
    { 1, QStringLiteral("reset_vel_to_gps"),           colorYellow},
    { 2, QStringLiteral("reset_vel_to_flow"),          colorYellow},
    { 3, QStringLiteral("reset_vel_to_vision"),        colorYellow},
    { 4, QStringLiteral("reset_vel_to_zero"),          colorYellow},
    { 5, QStringLiteral("reset_pos_to_last_known"),    colorYellow},
    { 6, QStringLiteral("reset_pos_to_gps"),           colorYellow},
    { 7, QStringLiteral("reset_pos_to_vision"),        colorYellow},
    { 8, QStringLiteral("starting_gps_fusion"),        colorNone},
    { 9, QStringLiteral("starting_vision_pos_fusion"), colorNone},
    {10, QStringLiteral("starting_vision_vel_fusion"), colorNone},
    {11, QStringLiteral("starting_vision_yaw_fusion"), colorNone},
    {12, QStringLiteral("yaw_aligned_to_imu_gps"),    colorNone},
    {13, QStringLiteral("reset_hgt_to_baro"),         colorYellow},
    {14, QStringLiteral("reset_hgt_to_gps"),          colorYellow},
    {15, QStringLiteral("reset_hgt_to_rng"),          colorYellow},
    {16, QStringLiteral("reset_hgt_to_ev"),           colorYellow},
    {17, QStringLiteral("reset_pos_to_ext_obs"),      colorYellow},
    {18, QStringLiteral("reset_wind_to_ext_obs"),     colorYellow},
};

EkfBitModel::EkfBitModel(QObject *parent)
    : QAbstractListModel(parent)
{}

void EkfBitModel::setEntries(const QVector<EkfBitEntry> &entries)
{
    beginResetModel();
    m_entries = entries;
    endResetModel();
}

int EkfBitModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant EkfBitModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};

    const auto &e = m_entries.at(index.row());

    switch (role) {
    case BitRole:
        return e.bit;
    case NameRole:
        return e.name;
    case LabelRole:
        return QStringLiteral("%1 %2").arg(e.bit, -2).arg(e.name);
    case OnColorRole:
        return e.onColor;
    }
    return {};
}

QHash<int, QByteArray> EkfBitModel::roleNames() const
{
    return {
        {BitRole, "bit"},
        {NameRole, "name"},
        {LabelRole, "label"},
        {OnColorRole, "onColor"},
    };
}

EkfBitModel *EkfBitModel::createFilterControlStatusLo(QObject *parent)
{
    auto *m = new EkfBitModel(parent);
    m->setEntries(s_filterControlStatusLo);
    return m;
}

EkfBitModel *EkfBitModel::createFilterControlStatusHi(QObject *parent)
{
    auto *m = new EkfBitModel(parent);
    m->setEntries(s_filterControlStatusHi);
    return m;
}

EkfBitModel *EkfBitModel::createFaultStatus(QObject *parent)
{
    auto *m = new EkfBitModel(parent);
    m->setEntries(s_faultStatus);
    return m;
}

EkfBitModel *EkfBitModel::createEventStatus(QObject *parent)
{
    auto *m = new EkfBitModel(parent);
    m->setEntries(s_eventStatus);
    return m;
}
