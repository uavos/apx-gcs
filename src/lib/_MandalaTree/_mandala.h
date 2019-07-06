/*
 * Copyright (C) 2015 Aliaksei Stratsilatau <sa@uavos.com>
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
#ifndef MANDALA_FIELDS
#define MANDALA_FIELDS
#include <inttypes.h>
#include <sys/types.h>
#include <string.h>
#include "preprocessor.h"
#include <dmsg.h>
#include <events.h>
//=============================================================================
/*

GPS->DBUS(post_data [_dbus_msg]):
    MBUS(set_fields)->Mandala
    MBUS_NoMandala(pack)->DBUS(send_packet)

DBUS(read_packet):
    DBUS(forward_packet)
    MBUS(set_fields)->Mandala

Mandala(changed [field_ptr]):
    MBUS(pack)->DBUS(send_packet)

*/
//=============================================================================
// peferences
//#define MANDALA_FULL
//#undef MANDALA_FULL
#define MANDALA_PACKED_STRUCT __attribute__((packed))
#define MANDALA_TEXT
#define MANDALA_FLAGS
//#define MANDALA_DOUBLE
#define MANDALA_ITERATORS

#ifndef USE_FLOAT_TYPE
#define MANDALA_DOUBLE
#endif
//=============================================================================
// EVENTS
class _mandala_field;
struct evt_m_changed
{ //whenever value is changed, emit by mandala if flag set
    _mandala_field *f;
    evt_m_changed(Evt<evt_m_changed> *src, _mandala_field *a_f)
        : f(a_f)
    {
        src->signal(*this);
    }
};
struct evt_m_send
{ //whenever value is changed, emit by mandala if flag set
    _mandala_field *f;
    evt_m_send(Evt<evt_m_send> *src, _mandala_field *a_f)
        : f(a_f)
    {
        src->signal(*this);
    }
};
struct evt_m_received
{ //when was unpacked on reception, emit by unpacker
    _mandala_field *f;
    evt_m_received(Evt<evt_m_received> *src, _mandala_field *a_f)
        : f(a_f)
    {
        src->signal(*this);
    }
};
//=============================================================================
#include "MatrixMath.h"
using namespace matrixmath;
//=============================================================================
//-----------------------------------------------------------------------------
// basic types
//-----------------------------------------------------------------------------
#ifdef MANDALA_DOUBLE
typedef double _mandala_float;
#else
typedef float _mandala_float;
#endif
typedef uint16_t _mandala_index;
typedef uint32_t _mandala_uint;
typedef uint8_t _mandala_byte;
typedef uint8_t _mandala_bit;
typedef uint8_t _mandala_enum;
typedef uint8_t _mandala_sid;
//=============================================================================
//configurable base classes
class _mandala_field_base
{
public:
#ifdef MANDALA_TEXT
    virtual const char *name() const = 0;
    virtual const char *descr() const = 0;
    virtual const char *opt(uint8_t n) const = 0;
    virtual const char *path() const = 0;
    virtual const char *alias() const = 0;
#endif
#ifdef MANDALA_ITERATORS
    typedef enum {
        mf_float,
        mf_uint,
        mf_byte,
        mf_bit,
        mf_enum,
        mf_vec3,
        mf_vec2,
    } _mf_type;
    typedef enum {
        fmt_float_f4,
        fmt_float_f2,
        fmt_float_u1,
        fmt_float_u01,
        fmt_float_u001,
        fmt_float_u10,
        fmt_float_u100,
        fmt_float_s1,
        fmt_float_s01,
        fmt_float_s001,
        fmt_float_s10,
        fmt_uint_u4,
        fmt_uint_u2,
        fmt_byte_,
        fmt_enum_,
        fmt_bit_,

        fmt_sid_,
    } _ext_fmt;
    virtual _mf_type type(void) const = 0;
    virtual _ext_fmt ext_fmt(void) = 0;
#endif
};

class _mandala_field : public _mandala_field_base
{
public:
    virtual _mandala_index index() const = 0;
    //value
    virtual _mandala_float toFloat() const = 0;
    virtual void fromFloat(const _mandala_float &v) = 0;
    virtual _mandala_uint toUInt() const = 0;
    virtual void fromUInt(const _mandala_uint &v) = 0;
#ifdef MANDALA_FLAGS
    _mandala_field() { flags.all = 0; }
    struct
    {
        union {
            uint16_t all;
            struct
            {
                uint8_t changed : 1;     //was changed (reset by others)
                uint8_t used : 1;        //was ever written
                uint8_t unpacked : 1;    //was ever unpacked (received)
                uint8_t report : 1;      //report changes
                uint8_t evt_changed : 1; //emit change event
                uint8_t evt_send : 1;    //emit send on change event
                //unpacking
                uint8_t toggle : 1;
                uint8_t prio : 2;
            };
        };
    } MANDALA_PACKED_STRUCT flags;
#endif
};
//=============================================================================
class _mandala : public Evt<evt_m_changed>, public Evt<evt_m_send>, public Evt<evt_m_received>
{
public:
//-----------------------------------------------------------------------------
// indexes _mandala::index_ap_sensor_imu_gyro_x
//-----------------------------------------------------------------------------
#define MIDX_MASK ((1 << 12) - 1)    //12 bit
#define MIDX_MASK_STREAM ((1 << 15)) //idx is a stream

//internal tree use
#define GRP1_MASK ((1 << 10) - 1) //9-4=  5bit (32 groups)
#define GRP2_MASK ((1 << 5) - 1)  //      5bit (32 fields)
#define MGRP1_BEGIN(...) \
    ATPASTE2(index_, MGRP1), \
        ATPASTE3(index_, MGRP1, _next) = (ATPASTE2(index_, MGRP1) & ~GRP1_MASK) - 1,
#define MGRP1_END \
    ATPASTE3(index_, MGRP1, _end) = (ATPASTE2(index_, MGRP1) & ~GRP1_MASK) + (GRP1_MASK + 1),
#define MGRP2_BEGIN(...) \
    ATPASTE4(index_, MGRP1, _, MGRP2), \
        ATPASTE5(index_, MGRP1, _, MGRP2, _next) = (ATPASTE4(index_, MGRP1, _, MGRP2) & ~GRP2_MASK) \
                                                   - 1,
#define MGRP2_END \
    ATPASTE5(index_, MGRP1, _, MGRP2, _end) = (ATPASTE4(index_, MGRP1, _, MGRP2) & ~GRP2_MASK) \
                                              + (GRP2_MASK + 1),

#define MFIELD_INDEX(aname) ATPASTE6(index_, MGRP1, _, MGRP2, _, aname)
#define MFIELD(atype, aname, ...) MFIELD_INDEX(aname),
#define MFENUM(atype, aname, ...) MFIELD_INDEX(aname),
    enum {
#include "_mandala.h"
    };

//enums
#define MFENUMV(aname, ename, descr, value) ATPASTE3(aname, _, ename) = value,
    enum {
#include "_mandala.h"
    };

    //-----------------------------------------------------------------------------
    // interfaces
    //-----------------------------------------------------------------------------
    class _group;

    class _group_base
    {
    public:
#ifdef MANDALA_TEXT
        virtual const char *name() const = 0;
        virtual const char *descr() const = 0;
#endif
#ifdef MANDALA_ITERATORS
        //iterators
        virtual _group *next_group(const _group *g) = 0;
        virtual _mandala_field *next_field(_mandala_field *f) = 0;
#endif
    };

    class _group : public _group_base
    {
    public:
        virtual _mandala_index index() const = 0;
        virtual _group *group(const _mandala_index idx) = 0;
        virtual _mandala_field *field(const _mandala_index idx) = 0;
    };

    //field template by value type
    template<class T>
    class _mandala_field_t : public _mandala_field
    {
    public:
        _mandala_field_t()
            : _mandala_field()
            , m_value(0)
        {}

        T &operator=(const T &v) { return setValue(v); }
        operator T() const { return m_value; }

        const T &value() const { return m_value; }
        //T & value() {return m_value;}

        //interface
        _mandala_float toFloat() const { return value(); }
        void fromFloat(const _mandala_float &v) { setValue(v); }
        _mandala_uint toUInt() const { return value(); }
        void fromUInt(const _mandala_uint &v) { setValue(v); }

        bool setValueLocal(const T &v)
        {
#ifdef MANDALA_FLAGS
            flags.used = 1;
#endif
            if (m_value == v)
                return false;
            m_value = v;
#ifdef MANDALA_FLAGS
            flags.changed = 1;
#endif
            return true;
        }
        void callEvents()
        {
#ifdef MANDALA_FLAGS
            if (flags.report)
                reportChanged();
            if (flags.evt_changed)
                evt_m_changed(m_ptr(), this);
            if (flags.evt_send)
                evt_m_send(m_ptr(), this);
#endif
        }

    protected:
        T m_value;
        T &setValue(const T &v)
        {
            if (!setValueLocal(v))
                return m_value;
            callEvents();
            return m_value;
        }
        virtual _mandala *m_ptr() const = 0;
        void reportChanged() const
        {
#if defined MANDALA_FLAGS && defined MANDALA_TEXT
            char s[64] = "";
            strcpy(s, descr());
            if (type() == mf_enum) {
                dmsg("%s: %s\n", descr(), opt(0x80 | (uint8_t) m_value));
                return;
            }
            const char *pos_slash = rindex(s, '/');
            if (pos_slash) {
                char *s_slash = rindex(s, '/');
                char *s_space = rindex(s, ' ');
                *s_slash = '\0';
                *s_space = '\0';
                dmsg("%s %s\n", s, (m_value > 0) ? (s_space + 1) : (s_slash + 1));
                s[0] = '\0';
                return;
            }
            char *pos_space = rindex(s, '[');
            if (pos_space)
                *(pos_space - 1) = '\0';
            dmsg("%s %s\n", s, (m_value > 0) ? "on" : "off");
#endif
        }
    };

    typedef _mandala_field_t<_mandala_float> _mandala_field_float_t;
    typedef _mandala_field_t<_mandala_uint> _mandala_field_uint_t;
    typedef _mandala_field_t<_mandala_byte> _mandala_field_byte_t;
    typedef _mandala_field_t<_mandala_bit> _mandala_field_bit_t;
    typedef _mandala_field_t<_mandala_enum> _mandala_field_enum_t;

    class _mandala_field_vec_t : public _mandala_field
    {
    public:
#ifdef MANDALA_TEXT
        const char *alias() const { return ""; }
        const char *opt(uint8_t n) const
        {
            (void) n;
            return "";
        }
#endif
#ifdef MANDALA_ITERATORS
        const char *name() const { return ""; }
        const char *descr() const { return ""; }
        const char *path() const { return ""; }
        _ext_fmt ext_fmt(void) { return fmt_float_f4; }
#endif
        _mandala_float toFloat() const { return 0; }
        void fromFloat(const _mandala_float &v) { (void) v; }
        _mandala_uint toUInt() const { return 0; }
        void fromUInt(const _mandala_uint &v) { (void) v; }
        virtual _mandala *m_ptr() const = 0;
        void callEvents()
        {
#ifdef MANDALA_FLAGS
            if (flags.evt_changed)
                evt_m_changed(m_ptr(), this);
            if (flags.evt_send)
                evt_m_send(m_ptr(), this);
#endif
        }
    };

    //generic unnamed field type (for small MCUs)
    class _value : public _mandala_field_t<_mandala_float>
    {
    public:
        _mandala_float &operator=(const _mandala_float &v) { return setValue(v); }
        _mandala_index index() const { return 0xFFFF; }
#ifdef MANDALA_TEXT
        const char *name() const { return ""; }
        const char *descr() const { return ""; }
        const char *opt(uint8_t n) const
        {
            (void) n;
            return "";
        }
        const char *path() const { return ""; }
        const char *alias() const { return ""; }
#endif
#ifdef MANDALA_ITERATORS
        _mf_type type(void) const { return mf_float; }
        _ext_fmt ext_fmt(void) { return fmt_float_f4; }
#endif
    };

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"

//fields typedefs
#define MFIELD_VAR(aname) MGRP1.MGRP2.aname
#define MFIELD_TYPE(aname) ATPASTE2(_, aname)

#define MFIELD(atype, aname, adescr, aalias, afmt, ...) \
    MFIELD_IMPL(MFIELD_INDEX(aname), atype, aname, adescr, aname, MFIELD_VAR(aname), aalias, afmt) \
    aname;

#define MFIELD_IMPL_BEGIN(aindex, atype, aname, adescr, atextname, apath, aalias, afmt) \
    class MFIELD_TYPE(aname) \
        : public _mandala_field_##atype##_t \
    { \
    public: \
        _mandala_index index() const { return aindex; } \
        const char *name() const { return ASTRINGZ(atextname); } \
        const char *descr() const { return adescr; } \
        const char *path() const { return ASTRINGZ(apath); } \
        const char *alias() const { return ASTRINGZ(aalias); } \
        _mf_type type(void) const { return mf_##atype; } \
        _ext_fmt ext_fmt(void) { return fmt_##atype##_##afmt; } \
        _mandala_##atype &operator=(const _mandala_##atype &v) { return setValue(v); } \
        enum { idx = aindex }; \
        _mandala *m_ptr() const \
        { \
            return (_mandala *) ((size_t) this - (size_t) & (((_mandala *) 0)->apath)); \
        }

#define MFIELD_IMPL(aindex, atype, aname, adescr, atextname, apath, aalias, afmt) \
    MFIELD_IMPL_BEGIN(aindex, atype, aname, adescr, atextname, apath, aalias, afmt) \
    const char *opt(uint8_t n) const \
    { \
        (void) n; \
        return ""; \
    } \
    }

//enums
#define MFENUM(atype, aname, adescr, aalias, afmt, ...) \
    MFIELD_IMPL_BEGIN(MFIELD_INDEX(aname), \
                      atype, \
                      aname, \
                      adescr, \
                      aname, \
                      MFIELD_VAR(aname), \
                      aalias, \
                      afmt) \
    const char *opt(uint8_t n) const \
    { \
        switch (n & 0x7F) { \
        default: \
            return "";

#define MFENUMV(avname, aname, adescr, anum) \
    case anum: \
        return (n & 0x80) ? adescr : ASTRINGZ(aname);

#define MFENUM_END(aname) \
    } \
    } \
    } \
    aname;

//vector fast access
#define MFVEC3(aname, v1, v2, v3) \
    class MFIELD_TYPE(aname) \
        : public _mandala_field_vec_t \
    { \
    public: \
        _mf_type type(void) const { return mf_vec3; } \
        _mandala_index index() const { return m_ptr()->MFIELD_VAR(v1).index(); } \
        _mandala *m_ptr() const \
        { \
            return (_mandala *) ((size_t) this - (size_t) & (((_mandala *) 0)->MFIELD_VAR(aname))); \
        } \
        operator Vector<3, _mandala_float>() const \
        { \
            return Vector<3, _mandala_float>(m_ptr()->MFIELD_VAR(v1).value(), \
                                             m_ptr()->MFIELD_VAR(v2).value(), \
                                             m_ptr()->MFIELD_VAR(v3.value())); \
        } \
        MFIELD_TYPE(aname) &operator=(const Vector<3, _mandala_float> &v) \
        { \
            m_ptr()->MFIELD_VAR(v1) = v[0]; \
            m_ptr()->MFIELD_VAR(v2) = v[1]; \
            m_ptr()->MFIELD_VAR(v3) = v[2]; \
            callEvents(); \
            return *this; \
        } \
    } aname;

#define MFVEC2(aname, v1, v2) \
    class MFIELD_TYPE(aname) \
        : public _mandala_field_vec_t \
    { \
    public: \
        _mf_type type(void) const { return mf_vec2; } \
        _mandala_index index() const { return m_ptr()->MFIELD_VAR(v1).index(); } \
        _mandala *m_ptr() const \
        { \
            return (_mandala *) ((size_t) this - (size_t) & (((_mandala *) 0)->MFIELD_VAR(aname))); \
        } \
        operator Vector<2, _mandala_float>() const \
        { \
            return Vector<2, _mandala_float>(m_ptr()->MFIELD_VAR(v1).value(), \
                                             m_ptr()->MFIELD_VAR(v2).value()); \
        } \
        MFIELD_TYPE(aname) &operator=(const Vector<2, _mandala_float> &v) \
        { \
            m_ptr()->MFIELD_VAR(v1) = v[0]; \
            m_ptr()->MFIELD_VAR(v2) = v[1]; \
            callEvents(); \
            return *this; \
        } \
    } aname;

//groups
#define MGRP_IMPL(aname, adescr) \
    class ATPASTE3(_, aname, _base) \
        : public _group \
    { \
    public: \
        const char *name() const { return ASTRINGZ(aname); } \
        const char *descr() const { return adescr; }

#define MGRP1_BEGIN(adescr, ...) MGRP_IMPL(MGRP1, adescr)
#define MGRP1_END \
    } \
    ;
#define MGRP2_BEGIN(adescr, ...) MGRP_IMPL(MGRP2, adescr)
#define MGRP2_END \
    } \
    ;

#include "_mandala.h"

#pragma GCC diagnostic pop

//grp2
#define MGRP_IMPL(aname, adescr) \
    class ATPASTE3(_, aname, _base_grp2) \
        : public ATPASTE3(_, aname, _base) \
    { \
    public:

#define MGRP1_BEGIN(adescr, ...) MGRP_IMPL(MGRP1, adescr)
#define MGRP1_END \
    } \
    ;
#define MGRP2_BEGIN(adescr, ...) \
    class ATPASTE2(_, MGRP2) \
        : public ATPASTE3(_, MGRP2, _base) \
    { \
    public: \
        _mandala_index index() const { return ATPASTE5(index_, MGRP1, _, MGRP2, _next) + 1; } \
        _group *group(const _mandala_index idx) \
        { \
            (void) idx; \
            return NULL; \
        } \
        _group *next_group(const _group *g) \
        { \
            (void) g; \
            return NULL; \
        } \
        _mandala_field *next_field(_mandala_field *f) \
        { \
            if (!f) \
                return field(index()); \
            _mandala_index i = f->index() + 1; \
            return field(i); \
        } \
        _mandala_field *field(const _mandala_index idx) \
        { \
            switch (idx) { \
            default: \
                return NULL;
#define MGRP2_END \
    } \
    } \
    } \
    MGRP2;

#define MFIELD(atype, aname, ...) \
    case MFIELD_INDEX(aname): \
        return &(aname);
#define MFENUM(atype, aname, ...) \
    case MFIELD_INDEX(aname): \
        return &(aname);

#include "_mandala.h"

//grp1
#define MGRP_IMPL(aname, adescr) \
    class ATPASTE3(_, aname, _base_grp1) \
        : public ATPASTE3(_, aname, _base_grp2) \
    { \
    public:

#define MGRP1_BEGIN(adescr, ...) \
    class ATPASTE2(_, MGRP1) \
        : public ATPASTE3(_, MGRP1, _base_grp2) \
    { \
    public: \
        _mandala_index index() const { return ATPASTE3(index_, MGRP1, _next) + 1; } \
        _mandala_field *field(const _mandala_index idx) \
        { \
            _group *g = group(idx); \
            if (!g) \
                return NULL; \
            return g->field(idx); \
        } \
        _group *next_group(const _group *g) \
        { \
            if (!g) \
                return group(ATPASTE3(index_, MGRP1, _next) + 1); \
            return group((g->index() & ~GRP2_MASK) + (GRP2_MASK + 1)); \
        } \
        _mandala_field *next_field(_mandala_field *f) \
        { \
            if (!f) \
                return field(index()); \
            _mandala_index i = f->index() + 1; \
            f = field(i); \
            if (f) \
                return f; \
            i = (i & (~GRP2_MASK)) + (GRP2_MASK + 1); \
            return field(i); \
        } \
        _group *group(const _mandala_index idx) \
        { \
            switch (idx & ~GRP2_MASK) { \
            default: \
                return NULL;

#define MGRP1_END \
    } \
    } \
    } \
    MGRP1;

#define MGRP2_BEGIN(adescr, ...) \
    case ATPASTE5(index_, MGRP1, _, MGRP2, _next) + 1: \
        return &(MGRP2);

#include "_mandala.h"

    //-----------------------------------------------------------------------------
    // METHODS
    //-----------------------------------------------------------------------------
public:
    // Iterators
    _mandala_field *field(const _mandala_index index)
    {
        _group *g = group(index);
        if (!g)
            return NULL;
        g = g->group(index);
        if (!g)
            return NULL;
        return g->field(index);
    }
    _group *group(const _mandala_index index)
    {
        switch (index & ~GRP1_MASK) {
        default:
            return NULL;
#define MGRP1_BEGIN(adescr, ...) \
    case ATPASTE3(index_, MGRP1, _next) + 1: \
        return &MGRP1;
#include "_mandala.h"
        }
    }
    _mandala_field *next_field(_mandala_field *f)
    {
        _mandala_index i = f ? f->index() + 1 : 0;
        f = field(i);
        if (f)
            return f;
        //try to inc grp2
        i = (i & (~GRP2_MASK)) + (GRP2_MASK + 1);
        f = field(i);
        if (f)
            return f;
        //try to inc grp1
        i = (i & (~GRP1_MASK)) + (GRP1_MASK + 1);
        return field(i);
    }
    _group *next_group(_group *g)
    {
        if (!g)
            return group(0);
        return group((g->index() & ~GRP1_MASK) + (GRP1_MASK + 1));
    }

    _mandala_float value(const _mandala_index index)
    {
        _mandala_field *f = field(index);
        return f ? f->toFloat() : 0;
    }
    void setValue(const _mandala_index index, const _mandala_float v)
    {
        _mandala_field *f = field(index);
        if (!f)
            return;
        f->fromFloat(v);
    }
    void setValue(const _mandala_index index, const _mandala_uint v)
    {
        _mandala_field *f = field(index);
        if (!f)
            return;
        f->fromUInt(v);
    }

    // static helpers
public:
    static _mandala_float &boundAngle(_mandala_float &v, _mandala_float span = 180.0)
    {
        const _mandala_float dspan = span * 2.0;
        v = v - floor(v / dspan + 0.5) * dspan;
        return v;
    }
    //===========================================================================
    static Vector<3, _mandala_float> &boundAngle(Vector<3, _mandala_float> &v,
                                                 _mandala_float span = 180.0)
    {
        _mandala::boundAngle(v[0], span);
        _mandala::boundAngle(v[1], span);
        _mandala::boundAngle(v[2], span);
        return v;
    }
    //===========================================================================
    static _mandala_float boundAngle360(_mandala_float v)
    {
        while (v < 0)
            v += 360.0;
        while (v >= 360.0)
            v -= 360.0;
        return v;
    }
    //===========================================================================
};
#undef MFIELD_TYPE
#undef MFIELD_VAR
#undef MFIELD_INDEX
//=============================================================================
#endif //MANDALA_FIELDS header
//=============================================================================
// LIST INCLUDE PART
//=============================================================================
#ifndef MGRP1_BEGIN
#define MGRP1_BEGIN(...)
#endif
#ifndef MGRP1_END
#define MGRP1_END
#endif
#ifndef MGRP2_BEGIN
#define MGRP2_BEGIN(...)
#endif
#ifndef MGRP2_END
#define MGRP2_END
#endif
#ifndef MFIELD
#define MFIELD(...)
#endif
#ifndef MFENUM
#define MFENUM(...)
#endif
#ifndef MFENUMV
#define MFENUMV(...)
#endif
#ifndef MFENUM_END
#define MFENUM_END(...)
#endif
#ifndef MFVEC3
#define MFVEC3(...)
#endif
#ifndef MFVEC2
#define MFVEC2(...)
#endif
//=============================================================================
#define MGRP1 sns
MGRP1_BEGIN("Sensors")

#define MGRP2 imu
MGRP2_BEGIN("Inertial Measurement Unit")
MFVEC3(acc, ax, ay, az)
MFIELD(float, ax, "Acceleration X [m/s2]", , f2)
MFIELD(float, ay, "Acceleration Y [m/s2]", , f2)
MFIELD(float, az, "Acceleration Z [m/s2]", , f2)
MFVEC3(gyro, gx, gy, gz)
MFIELD(float, gx, "Angular rate X [deg/s]", , f2)
MFIELD(float, gy, "Angular rate Y [deg/s]", , f2)
MFIELD(float, gz, "Angular rate Z [deg/s]", , f2)
MFVEC3(mag, hx, hy, hz)
MFIELD(float, hx, "Magnetic field X [a.u.]", , f2)
MFIELD(float, hy, "Magnetic field Y [a.u.]", , f2)
MFIELD(float, hz, "Magnetic field Z [a.u.]", , f2)
MFIELD(float, temp, "IMU temperature [C]", , f2)

//MFVECT(float,  acc,x,y,z, "Acceleration [m/s2]",                                Ax,Ay,Az, f2)
//MFVECT(float,  gyro,x,y,z,"Angular rate [deg/s]",                               p,q,r, f2)
//MFVECT(float,  mag,x,y,z, "Magnetic field [a.u.]",                              Hx,Hy,Hz, f2)
MGRP2_END
#undef MGRP2

#define MGRP2 gps
MGRP2_BEGIN("Global Positioning System")
MFVEC3(pos, lat, lon, hmsl)
MFIELD(float, lat, "GPS latitude [deg]", gps_lat, f4)
MFIELD(float, lon, "GPS latitude [deg]", gps_lon, f4)
MFIELD(float, hmsl, "GPS altitude MSL [m]", gps_hmsl, f4)
MFVEC3(vel, vn, ve, vd)
MFIELD(float, vn, "GPS velocity North [m/s]", gps_Vnorth, f2)
MFIELD(float, ve, "GPS velocity East [m/s]", gps_Veast, f2)
MFIELD(float, vd, "GPS velocity Down [m/s]", gps_Vdown, f2)
//MFVECT(float,  pos,lat,lon,hmsl, "GPS position [deg,deg,m]",                    gps_lat,gps_lon,gps_hmsl, f4)
//MFVECT(float,  vel,N,E,D,        "GPS velocity [m/s]",                          gps_Vnorth,gps_Veast,gps_Vdown, f2)
MFIELD(uint, utc, "GPS UTC Time from 1970 1st Jan [sec]", gps_time, u4)
MFIELD(byte, sv, "GPS Satellites visible [number]", gps_SV, )
MFIELD(byte, su, "GPS Satellites used [number]", gps_SU, )
MFIELD(byte, jcw, "GPS CW Jamming level [0..255]", gps_jcw, )
MFENUM(enum, jstate, "GPS Jamming", gps_jstate, )
MFENUMV(jstate, off, "disabled", 0)
MFENUMV(jstate, ok, "ok", 1)
MFENUMV(jstate, warn, "warning", 2)
MFENUMV(jstate, critical, "critical warning", 3)
MFENUM_END(jstate)
MGRP2_END
#undef MGRP2

#define MGRP2 air
MGRP2_BEGIN("Aerodynamic sensors")
MFIELD(float, pt, "Airspeed [m/s]", spdpt, f2)
MFIELD(float, ps, "Barometric altitude [m]", altps, f4)
MFIELD(float, vario, "Barometric variometer [m/s]", vario, f2)
MFIELD(float, ptemp, "Pitot probe temperature [C]", , s1)
MFIELD(float, slip, "Slip [deg]", slip, f2)
MFIELD(float, aoa, "Angle of attack [deg]", attack, f2)
MFIELD(float, buo, "Blimp ballonet pressure [kPa]", buoyancy, f2)
MGRP2_END
#undef MGRP2

#define MGRP2 agl
MGRP2_BEGIN("Distance to ground")
MFIELD(float, ultrasonic, "Ultrasonic altimeter [m]", , f2)
MFIELD(float, laser, "Laser altimeter [m]", , f2)
MFIELD(float, radio, "Radio altimeter [m]", , f4)
MFIELD(float, proximity, "Proximity sensor [m]", , f2)
MFIELD(float, touch, "Gear force sensor [N]", , f2)
MGRP2_END
#undef MGRP2

#define MGRP2 lps
MGRP2_BEGIN("Local Positioning System")
MFVEC3(pos, x, y, z)
MFIELD(float, x, "Local position sensor X [m]", , f2)
MFIELD(float, y, "Local position sensor Y [m]", , f2)
MFIELD(float, z, "Local position sensor Z [m]", , f2)
MFVEC3(vel, vx, vy, vz)
MFIELD(float, vx, "Local velocity sensor X [m/s]", , f2)
MFIELD(float, vy, "Local velocity sensor Y [m/s]", , f2)
MFIELD(float, vz, "Local velocity sensor Z [m/s]", , f2)
//MFVECT(float, pos,x,y,z, "Local position sensor [m]",                           ,,, f2)
//MFVECT(float, vel,x,y,z, "Local velocity sensor [m/s]",                         ,,, f2)
MGRP2_END
#undef MGRP2

#define MGRP2 ils
MGRP2_BEGIN("Instrument Landing System")
MFIELD(float, HDG, "ILS heading to VOR1 [deg]", , f2)
MFIELD(uint, DME, "ILS distance to VOR1 [m]", , u2)
MFIELD(float, RSS, "ILS signal strength [0..1]", , f2)
MFIELD(float, dHDG, "ILS error heading [deg]", , f2)
MFIELD(float, alt, "ILS error altitude [m]", , f2)
MGRP2_END
#undef MGRP2

#define MGRP2 ldp
MGRP2_BEGIN("Landing Platform sensors")
MFVEC3(pos, lat, lon, hmsl)
MFIELD(float, lat, "Platform latitude [deg]", , f4)
MFIELD(float, lon, "Platform latitude [deg]", , f4)
MFIELD(float, hmsl, "Platform altitude MSL [m]", , f4)
MFVEC3(vel, vn, ve, vd)
MFIELD(float, vn, "Platform velocity North [m/s]", , f2)
MFIELD(float, ve, "Platform velocity East [m/s]", , f2)
MFIELD(float, vd, "Platform velocity Down [m/s]", , f2)
MFVEC3(att, roll, pitch, yaw)
MFIELD(float, roll, "Platform attitude roll [deg]", , f2)
MFIELD(float, pitch, "Platform attitude pitch [deg]", , f2)
MFIELD(float, yaw, "Platform attitude yaw [deg]", , f2)
//MFVECT(float,  pos,lat,lon,hmsl,   "Platform position [deg,deg,m]",             ,,, f4)
//MFVECT(float,  vel,N,E,D,          "Platform velocity [m/s]",                   ,,, f2)
//MFVECT(float,  att,roll,pitch,yaw, "Platform attitude [deg]",                   ,,, f2)
MGRP2_END
#undef MGRP2

#define MGRP2 radar
MGRP2_BEGIN("Radar target tracking")
MFIELD(byte, id, "Radar target ID", , )
MFIELD(float, HDG, "Radar heading to target [deg]", , f2)
MFIELD(uint, DME, "Radar distance to target [m]", , u2)
MFIELD(float, vel, "Radar target velocity [m/s]", , f2)
MGRP2_END
#undef MGRP2

#define MGRP2 engine
MGRP2_BEGIN("Engine sensors", engine)
MFIELD(uint, rpm, "Engine RPM [1/min]", rpm, u2)
MFIELD(uint, rpm_prop, "Prop RPM [1/min]", , u2)
MFIELD(float, fuel, "Fuel capacity [l]", fuel, f2)
MFIELD(float, frate, "Fuel flow rate [l/h]", frate, f2)
MFIELD(float, ET, "Engine temperature [C]", ET, u1)
MFIELD(float, OT, "Oil temperature [C]", OT, u1)
MFIELD(float, OP, "Oil pressure [atm]", OP, u01)
MFIELD(float, EGT1, "Exhaust 1 temperature [C]", EGT, u10)
MFIELD(float, EGT2, "Exhaust 2 temperature [C]", , u10)
MFIELD(float, EGT3, "Exhaust 3 temperature [C]", , u10)
MFIELD(float, EGT4, "Exhaust 4 temperature [C]", , u10)
MFIELD(float, MAP, "MAP pressure [Pa]", , f2)
MFIELD(float, IAP, "Intake air box pressure [kPa]", , f2)
MFIELD(bit, TCAUT, "Turbocharger caution/ok", , )
MFIELD(bit, TWARN, "Turbocharger warning/ok", , )
MFIELD(bit, start, "Engine start procedure trigger", ctrb_starter, )
MFIELD(bit, error, "Engine error/ok", sb_eng_err, )
MGRP2_END
#undef MGRP2

#define MGRP2 power
MGRP2_BEGIN("System power", vsense)
MFIELD(float, Ve, "System battery voltage [v]", Ve, f2)
MFIELD(float, Ie, "System current [A]", Ie, u001)
MFIELD(float, Vs, "Servo battery voltage [v]", Vs, f2)
MFIELD(float, Is, "Servo current [A]", Is, u01)
MFIELD(float, Vp, "Payload battery voltage [v]", Vp, f2)
MFIELD(float, Ip, "Payload current [A]", Ip, u01)
MFIELD(float, Vm, "ECU battery voltage [v]", Vm, f2)
MFIELD(float, Im, "ECU current [A]", Im, u1)
MFIELD(bit, shutdown, "System trigger shutdown/on", sb_shutdown, )
MGRP2_END
#undef MGRP2

#define MGRP2 bat
MGRP2_BEGIN("Battery management", bat)
MFIELD(float, Vbat1, "Battery 1 voltage [v]", , f2)
MFIELD(float, Ib1, "Battery 1 current [A]", , u01)
MFIELD(float, BT1, "Battery 1 temperature [C]", , s1)
MFIELD(bit, err_b1, "Battery 1 error/ok", , )
MFIELD(float, Vbat2, "Battery 2 voltage [v]", , f2)
MFIELD(float, Ib2, "Battery 2 current [A]", , u01)
MFIELD(float, BT2, "Battery 2 temperature [C]", , s1)
MFIELD(bit, err_b2, "Battery 2 error/ok", , )
MFIELD(float, Vgen, "Power generator voltage [v]", , f2)
MFIELD(float, Igen, "Generator current [A]", , u01)
MFIELD(float, Ichg, "Charging current [A]", , u01)
MFIELD(bit, err_gen, "Power generator error/ok", sb_gen_err, )
MGRP2_END
#undef MGRP2

#define MGRP2 temp
MGRP2_BEGIN("Temperature sensors", temp)
MFIELD(float, AT, "Ambient temperature [C]", AT, s1)
MFIELD(float, RT, "Room temperature [C]", RT, s1)
MGRP2_END
#undef MGRP2

#define MGRP2 dlink
MGRP2_BEGIN("Datalink radio sensors", dlink)
MFIELD(float, RSS, "Modem signal strength [0..1]", RSS, f2)
MFIELD(float, MT, "Modem temperature [C]", MT, s1)
MFIELD(float, HDG, "Modem heading to transmitter [deg]", , f2)
MFIELD(uint, DME, "Modem distance to transmitter [m]", , u4)
MGRP2_END
#undef MGRP2

#define MGRP2 pilot
MGRP2_BEGIN("Pilot stick sensors")
MFIELD(byte, ovr, "RC override [0..255]", rc_override, )
MFIELD(float, roll, "RC roll [-1..+1]", rc_roll, s001)
MFIELD(float, pitch, "RC pitch [-1..+1]", rc_pitch, s001)
MFIELD(float, throttle, "RC throttle [0..1]", rc_throttle, s001)
MFIELD(float, yaw, "RC yaw [-1..+1]", rc_yaw, s001)
MGRP2_END
#undef MGRP2

#define MGRP2 str
MGRP2_BEGIN("Steering encoders")
MFIELD(float, vleft, "Steering velocity left [m/s]", , f2)
MFIELD(float, vright, "Steering velocity right [m/s]", , f2)
//MFVEC2(float, vel,left,right,     "Steering velocity [m/s]",                    ,, f2)
MFIELD(float, HDG, "Steering heading [deg]", , f2)
MGRP2_END
#undef MGRP2

#define MGRP2 ers
MGRP2_BEGIN("ERS sensors", ers)
MFIELD(bit, error, "ERS error/ok", sb_ers_err, )
MFIELD(bit, disarm, "ERS disarmed/armed", sb_ers_disarm, )
MGRP2_END
#undef MGRP2

#define MGRP2 gcu
MGRP2_BEGIN("Ground Station info")
MFVEC3(pos, lat, lon, hmsl)
MFIELD(float, lat, "GCU latitude [deg]", gcu_lat, f4)
MFIELD(float, lon, "GCU latitude [deg]", gcu_lon, f4)
MFIELD(float, hmsl, "GCU altitude MSL [m]", gcu_hmsl, f4)
MFVEC3(vel, vn, ve, vd)
MFIELD(float, vn, "GCU velocity North [m/s]", , f2)
MFIELD(float, ve, "GCU velocity East [m/s]", , f2)
MFIELD(float, vd, "GCU velocity Down [m/s]", , f2)
MFVEC3(att, roll, pitch, yaw)
MFIELD(float, roll, "GCU attitude roll [deg]", , f2)
MFIELD(float, pitch, "GCU attitude pitch [deg]", , f2)
MFIELD(float, yaw, "GCU attitude yaw [deg]", , f2)
//MFVECT(float, pos,lat,lon,hmsl,"GCU global position [deg,deg,m]",               gcu_lat,gcu_lon,gcu_hmsl, f4)
MFIELD(float, RSS, "GCU modem signal strength [0..1]", gcu_RSS, f2)
MFIELD(float, Ve, "GCU system battery voltage [v]", gcu_Ve, f2)
MFIELD(float, MT, "GCU modem temperature [C]", , s1)
MGRP2_END
#undef MGRP2

#define MGRP2 meteo
MGRP2_BEGIN("Meteo base station info")
MFIELD(float, windSpd, "wind speed [m/s]", , u01)
MFIELD(float, windHdg, "wind direction to [deg]", , f2)
MFIELD(float, temp, "Ground outside air temperature [C]", , s1)
MFIELD(float, altps, "barometric altitude on ground level [m]", , f2)
MGRP2_END
#undef MGRP2

MGRP1_END
#undef MGRP1
//-------------------------
#define MGRP1 ctr
MGRP1_BEGIN("Control outputs")

#define MGRP2 stab
MGRP2_BEGIN("Stability fast controls")
MFIELD(float, ail, "Ailerons [-1..+1]", ctr_ailerons, s001)
MFIELD(float, elv, "Elevator [-1..+1]", ctr_elevator, s001)
MFIELD(float, rud, "Rudder [-1..+1]", ctr_rudder, s001)
MFIELD(float, col, "Collective pitch [-1..+1]", ctr_collective, s001)
MFIELD(float, str, "Steering [-1..+1]", ctr_steering, s001)
MGRP2_END
#undef MGRP2

#define MGRP2 eng
MGRP2_BEGIN("Engine controls")
MFIELD(float, thr, "Throttle [0..1]", ctr_throttle, u001)
MFIELD(float, prop, "Prop pitch [-1..+1]", , s001)
MFIELD(float, mix, "Mixture [0..1]", ctr_mixture, u001)
MFIELD(float, tune, "Engine tuning [-1..1]", ctr_engine, s001)
MFIELD(float, vector, "Thrust vector [-1..+1]", , s001)
MFIELD(bit, starter, "Engine starter on/off", sw_starter, )
MFIELD(bit, horn, "Engine horn signal", ctrb_horn, )
MFIELD(bit, rev, "Thrust reverse on/off", ctrb_rev, )
MFIELD(bit, ign1, "Ignition 1 on/off", , )
MFIELD(bit, ign2, "Ignition 2 on/off", , )
MGRP2_END
#undef MGRP2

#define MGRP2 wing
MGRP2_BEGIN("Wing mechanization")
MFIELD(float, flaps, "Flaps [0..1]", ctr_flaps, u001)
MFIELD(float, airbrk, "Airbrakes [0..1]", ctr_airbrk, u001)
MFIELD(float, slats, "Wing slats [0..1]", , u001)
MFIELD(float, sweep, "Wing sweep [-1..+1]", ctr_sweep, s001)
MFIELD(float, buoyancy, "Buoyancy [-1..+1]", ctr_buoyancy, s001)
MGRP2_END
#undef MGRP2

#define MGRP2 brakes
MGRP2_BEGIN("Brakes system control")
MFIELD(float, brake, "Brake [0..1]", ctr_brake, u001)
MFIELD(float, brakeL, "Left brake [0..1]", ctr_brakeL, u001)
MFIELD(float, brakeR, "Right brake [0..1]", ctr_brakeR, u001)
MGRP2_END
#undef MGRP2

#define MGRP2 ers
MGRP2_BEGIN("Emergency Recovery System controls")
MFIELD(bit, launch, "ERS on/off", ctrb_ers, )
MFIELD(bit, rel, "Parachute released/locked", ctrb_rel, )
MGRP2_END
#undef MGRP2

#define MGRP2 gear
MGRP2_BEGIN("Landing Gear controls")
MFIELD(bit, retract, "Landing gear retracted/extracted", ctrb_gear, )
MGRP2_END
#undef MGRP2

#define MGRP2 fuel
MGRP2_BEGIN("Fuel pumps")
MFIELD(bit, pump, "Fuel pump on/off", ctrb_pump, )
MFIELD(bit, xfeed, "Crossfeed on/off", , )
MGRP2_END
#undef MGRP2

#define MGRP2 power
MGRP2_BEGIN("Power management controls")
MFIELD(bit, ap, "Avionics", power_ap, )
MFIELD(bit, servo, "Servo on/off", power_servo, )
MFIELD(bit, ignition, "Engine on/off", power_ignition, )
MFIELD(bit, payload, "Payload on/off", power_payload, )
MFIELD(bit, agl, "AGL sensor", power_agl, )
MFIELD(bit, xpdr, "XPDR on/off", power_xpdr, )
MFIELD(bit, satcom, "Satcom on/off", , )
MFIELD(bit, rfamp, "RF amplifier on/off", , )
MFIELD(bit, ils, "Instrument Landing System on/off", , )
MFIELD(bit, ice, "Anti-ice on/off", sw_ice, )
MGRP2_END
#undef MGRP2

#define MGRP2 light
MGRP2_BEGIN("Lighting")
MFIELD(bit, beacon, "Beacon light on/off", , )
MFIELD(bit, landing, "Landing lights on/off", sw_lights, )
MFIELD(bit, nav, "Navigation lights on/off", , )
MFIELD(bit, strobe, "Strobe light on/off", , )
MFIELD(bit, taxi, "Taxi lights on/off", sw_taxi, )
MGRP2_END
#undef MGRP2

#define MGRP2 door
MGRP2_BEGIN("Doors and latches")
MFIELD(bit, main, "Main door open/locked", , )
MFIELD(bit, drop, "Drop-off open/locked", ctrb_drp, )
MGRP2_END
#undef MGRP2

#define MGRP2 cam
MGRP2_BEGIN("Camera controls output")
MFIELD(float, ctr_roll, "Cam servo roll [-1..+1]", camctr_roll, s001)
MFIELD(float, ctr_pitch, "Cam servo pitch [-1..+1]", camctr_pitch, s001)
MFIELD(float, ctr_yaw, "Cam servo yaw [-1..+1]", camctr_yaw, s001)
//MFVECT(float, ctr,roll,pitch,yaw, "Cam servo [-1..+1]",                    camctr_roll,camctr_pitch,camctr_yaw, s001)
MFIELD(bit, rec, "Recording", camctrb_rec, )
MFIELD(bit, shot, "Snapshot", camctrb_shot, )
MFIELD(bit, ashot, "Series snapshots", camctrb_ashot, )
MGRP2_END
#undef MGRP2

#define MGRP2 ats
MGRP2_BEGIN("ATS controls output")
MFIELD(float, ctr_roll, "ATS servo roll [-1..+1]", atsctr_roll, s001)
MFIELD(float, ctr_pitch, "ATS servo pitch [-1..+1]", atsctr_pitch, s001)
MFIELD(float, ctr_yaw, "ATS servo yaw [-1..+1]", atsctr_yaw, s001)
//MFVECT(float, ctr,roll,pitch,yaw, "ATS servo [-1..+1]",                    atsctr_roll,atsctr_pitch,atsctr_yaw, s001)
MGRP2_END
#undef MGRP2

#define MGRP2 turret
MGRP2_BEGIN("Turret controls output")
MFIELD(float, ctr_roll, "Turret servo roll [-1..+1]", , s001)
MFIELD(float, ctr_pitch, "Turret servo pitch [-1..+1]", , s001)
MFIELD(float, ctr_yaw, "Turret servo yaw [-1..+1]", , s001)
//MFVECT(float, ctr,roll,pitch,yaw, "Turret servo [-1..+1]",                 ,,, s001)
MFIELD(bit, armed, "Turret armed/disarmed", , )
MFIELD(bit, shoot, "Turret shooting/standby", , )
MFIELD(bit, reload, "Turret reloading/reloaded", , )
MGRP2_END
#undef MGRP2

#define MGRP2 usr
MGRP2_BEGIN("User controls")
MFIELD(float, u1, "User control 1", ctr1, f2)
MFIELD(float, u2, "User control 2", ctr2, f2)
MFIELD(float, u3, "User control 3", ctr3, f2)
MFIELD(float, u4, "User control 4", ctr4, f2)
MFIELD(float, u5, "User control 5", ctr5, f2)
MFIELD(float, u6, "User control 6", ctr6, f2)
MFIELD(float, u7, "User control 7", ctr7, f2)
MFIELD(float, u8, "User control 8", ctr8, f2)
MGRP2_END
#undef MGRP2

MGRP1_END
#undef MGRP1
//-------------------------
#define MGRP1 state
MGRP1_BEGIN("System state")

#define MGRP2 ahrs
MGRP2_BEGIN("Attitude and position estimation")
MFVEC3(att, roll, pitch, yaw)
MFIELD(float, roll, "Attitude roll [deg]", roll, f2)
MFIELD(float, pitch, "Attitude pitch [deg]", pitch, f2)
MFIELD(float, yaw, "Attitude yaw [deg]", yaw, f2)
//MFVECT(float, att,roll,pitch,yaw,    "Attitude [deg]",                          roll,pitch,yaw, f2)
MFIELD(float, lat, "Latitude [deg]", , f4)
MFIELD(float, lon, "Longitude [deg]", , f4)
//MFVEC2(ne, north,east)
MFIELD(float, north, "Local position north [m]", , f4)
MFIELD(float, east, "Local position east [m]", , f4)
//MFVEC2(vel, north,east)
MFIELD(float, vn, "Local velocity north [m/s]", , f2)
MFIELD(float, ve, "Local velocity east [m/s]", , f2)
//MFVEC2(float, NE,N,E,   "Local position [m]",                                   pos_north,pos_east, f4)
//MFVEC2(float, vel,N,E,  "Local velocity [m/s]",                                 vel_north,vel_east, f2)
MFIELD(float, altitude, "Altitude [m]", altitude, f4)
MFIELD(float, course, "Moving direction [deg]", course, f2)
MFIELD(float, gSpeed, "Ground speed [m/s]", gSpeed, f2)
MGRP2_END
#undef MGRP2

#define MGRP2 flight
MGRP2_BEGIN("Flight parameters")
MFIELD(float, airspeed, "Airspeed [m/s]", airspeed, f2)
MFIELD(float, vspeed, "Vertical speed [m/s]", vspeed, f2)
MFIELD(float, ldratio, "Glide ratio [Lift/Drag]", ldratio, f2)
MFIELD(float, venergy, "Compensated variometer [m/s]", venergy, f2)
MFIELD(float, agl, "Above Ground Level altitude [m]", agl, f2)
MFIELD(float, vcas, "Airspeed derivative [m/s^2]", vcas, f2)
MFIELD(float, denergy, "Venergy derivative [m/s^2]", denergy, f2)
MGRP2_END
#undef MGRP2

#define MGRP2 status
MGRP2_BEGIN("Status of subsystems")
MFIELD(bit, rc, "RC on/off", status_rc, )
MFIELD(bit, gps, "GPS available/lost", status_gps, )
MFIELD(bit, agl, "AGL available/off", status_agl, )
MFIELD(bit, uplink, "Uplink available/lost", status_modem, )
MFIELD(bit, landed, "Vehicle landed/flying", status_landed, )
MFIELD(bit, touch, "Landing gear touchdown/floating", status_touch, )
MGRP2_END
#undef MGRP2

#define MGRP2 error
MGRP2_BEGIN("System errors and warnings")
MFIELD(bit, fatal, "Fatal error/ok", , )
MFIELD(bit, power, "Power supply error/ok", error_power, )
MFIELD(bit, engine, "Engine error/ok", , )
MFIELD(bit, rpm, "RPM sensor error/ok", error_rpm, )
MFIELD(bit, cas, "CAS error", error_cas, )
MFIELD(bit, pstatic, "Static pressure error/ok", error_pstatic, )
MFIELD(bit, gyro, "IMU gyros bias", error_gyro, )
MFIELD(bit, ers, "ERS error/ok", , )
MGRP2_END
#undef MGRP2

#define MGRP2 nav
MGRP2_BEGIN("Navigation parameters")
//MFVEC2(dxy, dx,dy)
MFIELD(float, dx, "Bodyframe delta X [m]", , f2)
MFIELD(float, dy, "Bodyframe delta Y [m]", , f2)
//MFVEC2(vxy, vx,vy)
MFIELD(float, vx, "Bodyframe velocity X [m/s]", , f2)
MFIELD(float, vy, "Bodyframe velocity Y [m/s]", , f2)
//MFVEC2(float, dXY,x,y,   "Bodyframe delta [m]",                                 dx,dy, f2)
//MFVEC2(float, vXY,x,y,   "Bodyframe velocity [m/s]",                            Vx,Vy, f2)
MFIELD(float, delta, "General delta (depends on mode) [m]", delta, f2)
MFIELD(float, tgHDG, "Current tangent heading [deg]", tgHDG, f2)
MFIELD(float, stab, "Stability [0..1]", stab, u001)
MFIELD(float, corr, "Correlator output [K]", , f2)
MFIELD(float, rwDelta, "Runway alignment [m]", rwDelta, f2)
MFIELD(float, rwDV, "Runway alignment velocity [m/s]", rwDV, f2)
MGRP2_END
#undef MGRP2

#define MGRP2 mission
MGRP2_BEGIN("Waypoint information")
MFIELD(float, DME, "Distance to waypoint [m]", dWPT, f2)
MFIELD(float, HDG, "Current waypoint heading [deg]", wpHDG, f2)
MFIELD(uint, ETA, "Estimated time of arrival [s]", ETA, u4)
MFIELD(byte, wpidx, "Current waypoint [0..255]", wpidx, )
MFIELD(byte, rwidx, "Current runway [0..255]", rwidx, )
MFIELD(byte, twidx, "Current taxiway [0..255]", twidx, )
MFIELD(byte, piidx, "Current point of interest [0..255]", piidx, )
MGRP2_END
#undef MGRP2

#define MGRP2 ils
MGRP2_BEGIN("Instrument Landing System status")
MFIELD(bit, armed, "ILS armed/off", , )
MFIELD(bit, approach, "ILS approach available/lost", , )
MFIELD(bit, offset, "ILS offset available/lost", , )
MFIELD(bit, platform, "ILS platform available/lost", , )
MGRP2_END
#undef MGRP2

#define MGRP2 home
MGRP2_BEGIN("Home point")
MFVEC3(pos, lat, lon, hmsl)
MFIELD(float, lat, "Home latitude [deg]", home_lat, f4)
MFIELD(float, lon, "Home latitude [deg]", home_lon, f4)
MFIELD(float, hmsl, "Home altitude MSL [m]", home_hmsl, f4)
//MFVECT(float,  pos,lat,lon,hmsl,"Home global position [deg,deg,m]",             home_lat,home_lon,home_hmsl, f4)
MFIELD(float, altps, "Barometric altitude on ground level [m]", altps_gnd, f2)
MFIELD(float, HDG, "Heading to home [deg]", homeHDG, f2)
MFIELD(float, dist, "Distance to home [m]", dHome, f2)
MFIELD(bit, set, "GPS initialized", status_home, )
MGRP2_END
#undef MGRP2

#define MGRP2 wind
MGRP2_BEGIN("Wind estimator")
MFIELD(float, speed, "wind speed [m/s]", windSpd, u01)
MFIELD(float, HDG, "wind direction to [deg]", windHdg, f2)
MFIELD(float, cas2tas, "CAS to TAS multiplier [K]", cas2tas, u001)
MGRP2_END
#undef MGRP2

#define MGRP2 downlink
MGRP2_BEGIN("Downlink parameters")
MFIELD(uint, period, "downlink period [ms]", dl_period, u2)
MFIELD(uint, timestamp, "downlink timestamp [ms]", dl_timestamp, u4)
MGRP2_END
#undef MGRP2

#define MGRP2 cam
MGRP2_BEGIN("Camera control system state")
MFVEC3(att, roll, pitch, yaw)
MFIELD(float, roll, "Cam attitude roll [deg]", cam_roll, f2)
MFIELD(float, pitch, "Cam attitude pitch [deg]", cam_pitch, f2)
MFIELD(float, yaw, "Cam attitude yaw [deg]", cam_yaw, f2)
//MFVECT(float, att,roll,pitch,yaw, "Cam attitude [deg]",                         cam_roll,cam_pitch,cam_yaw, f2)
MFIELD(uint, timestamp, "Cam timestamp [ms]", cam_timestamp, u4)
MFIELD(float, DME, "Distance to tracking object [m]", , f2)
MGRP2_END
#undef MGRP2

#define MGRP2 ats
MGRP2_BEGIN("ATS system state")
MFVEC3(att, roll, pitch, yaw)
MFIELD(float, roll, "ATS attitude roll [deg]", , f2)
MFIELD(float, pitch, "ATS attitude pitch [deg]", , f2)
MFIELD(float, yaw, "ATS attitude yaw [deg]", , f2)
MFIELD(float, enc_roll, "ATS encoder roll [deg]", atsenc_roll, f2)
MFIELD(float, enc_pitch, "ATS encoder pitch [deg]", atsenc_pitch, f2)
MFIELD(float, enc_yaw, "ATS encoder yaw [deg]", atsenc_yaw, f2)
//MFVECT(float, att,roll,pitch,yaw, "ATS attitude [deg]",                         ,,, f2)
//MFVECT(float, enc,roll,pitch,yaw, "ATS encoders [deg]",                         atsenc_roll,atsenc_pitch,atsenc_yaw, f4)
MFENUM(enum, mode, "ATS mode", ats_mode, )
MFENUMV(mode, track, "turret off", 0)
MFENUMV(mode, manual, "fixed position", 1)
MFENUM_END(mode)
MGRP2_END
#undef MGRP2

#define MGRP2 turret
MGRP2_BEGIN("Turret system state")
MFIELD(float, roll, "Turret attitude roll [deg]", , f2)
MFIELD(float, pitch, "Turret attitude pitch [deg]", , f2)
MFIELD(float, yaw, "Turret attitude yaw [deg]", , f2)
MFIELD(float, enc_roll, "Turret encoder roll [deg]", , f2)
MFIELD(float, enc_pitch, "Turret encoder pitch [deg]", , f2)
MFIELD(float, enc_yaw, "Turret encoder yaw [deg]", , f2)
//MFVECT(float, att,roll,pitch,yaw, "Turret attitude [deg]",                      ,,, f2)
//MFVECT(float, enc,roll,pitch,yaw, "Turret encoders [deg]",                      ,,, f4)
MFIELD(byte, ammo, "Turret ammo [0..255]", , )
MGRP2_END
#undef MGRP2

#define MGRP2 usr
MGRP2_BEGIN("User values")
MFIELD(float, u1, "User value 1", user1, f2)
MFIELD(float, u2, "User value 2", user2, f2)
MFIELD(float, u3, "User value 3", user3, f2)
MFIELD(float, u4, "User value 4", user4, f2)
MFIELD(float, u5, "User value 5", user5, f2)
MFIELD(float, u6, "User value 6", user6, f2)
MFIELD(float, u7, "User value 7", user7, f2)
MFIELD(float, u8, "User value 8", user8, f2)
MFIELD(float, u9, "User value 9", , f2)
MFIELD(float, u10, "User value 10", , f2)
MFIELD(float, u11, "User value 11", , f2)
MFIELD(float, u12, "User value 12", , f2)
MFIELD(float, u13, "User value 13", , f2)
MFIELD(float, u14, "User value 14", , f2)
MFIELD(float, u15, "User value 15", , f2)
MFIELD(float, u16, "User value 16", , f2)
MFIELD(bit, b1, "User bit 1", userb_1, )
MFIELD(bit, b2, "User bit 2", userb_2, )
MFIELD(bit, b3, "User bit 3", userb_3, )
MFIELD(bit, b4, "User bit 4", userb_4, )
MFIELD(bit, b5, "User bit 5", userb_5, )
MFIELD(bit, b6, "User bit 6", userb_6, )
MFIELD(bit, b7, "User bit 7", userb_7, )
MFIELD(bit, b8, "User bit 8", userb_8, )
MGRP2_END
#undef MGRP2

MGRP1_END
#undef MGRP1
//-------------------------
#define MGRP1 cmd
MGRP1_BEGIN("Command")

#define MGRP2 reg
MGRP2_BEGIN("Commanded regulator values")
MFIELD(float, roll, "Commanded roll [deg]", cmd_roll, f2)
MFIELD(float, pitch, "Commanded pitch [deg]", cmd_pitch, f2)
MFIELD(float, yaw, "Commanded yaw [deg]", cmd_yaw, f2)
MFVEC3(pos, lat, lon, hmsl)
MFIELD(float, lat, "Commanded latitude [deg]", , f4)
MFIELD(float, lon, "Commanded latitude [deg]", , f4)
MFIELD(float, hmsl, "Commanded altitude MSL [m]", , f4)
//MFVECT(float, att,roll,pitch,yaw, "Commanded attitude [deg]",                   cmd_roll,cmd_pitch,cmd_yaw, f2)
//MFVECT(float, pos,lat,lon,hmsl,   "Commanded global position [deg,deg,m]",      ,,, f4)
MFIELD(float, course, "Commanded course [deg]", cmd_course, f2)
MFIELD(float, altitude, "Commanded altitude [m]", cmd_altitude, f4)
MFIELD(float, airspeed, "Commanded airspeed [m/s]", cmd_airspeed, f2)
MFIELD(float, vspeed, "Commanded vspeed [m/s]", cmd_vspeed, f2)
MFIELD(float, slip, "Commanded slip [deg]", cmd_slip, f2)
MFIELD(uint, rpm, "Commanded RPM [1/m]", cmd_rpm, u2)
//MFVEC2(ne, north,east)
MFIELD(float, north, "Commanded position north [m]", , f4)
MFIELD(float, east, "Commanded position east [m]", , f4)
//MFVEC2(float, NE,N,E,             "Commanded position [m]",                     cmd_north,cmd_east, f4)
MGRP2_END
#undef MGRP2

#define MGRP2 op
MGRP2_BEGIN("Operation mode")
MFENUM(enum, mode, "flight mode", mode, )
MFENUMV(mode, EMG, "Realtime control", 0)
MFENUMV(mode, RPV, "Angles control", 1)
MFENUMV(mode, UAV, "Heading control", 2)
MFENUMV(mode, WPT, "Waypoints navigation", 3)
MFENUMV(mode, HOME, "Go back home", 4)
MFENUMV(mode, STBY, "Loiter around DNED", 5)
MFENUMV(mode, TAXI, "Taxi", 6)
MFENUMV(mode, TAKEOFF, "Takeoff", 7)
MFENUMV(mode, LANDING, "Landing", 8)
MFENUM_END(mode)
MFIELD(byte, stage, "Maneuver stage", stage, )
MFIELD(bit, cancel, "Cancel current procedure", , )
MFIELD(bit, inc, "Increment mission index", , )
MFIELD(bit, dec, "Decrement mission index", , )
MFIELD(float, rwAdj, "Runway displacement adjust [m]", rwAdj, s1)
MFENUM(enum, mtype, "Mission maneuver type", mtype, )
MFENUMV(mtype, hdg, "Heading navigation", 0)
MFENUMV(mtype, line, "Line navigation", 1)
MFENUM_END(mtype)
MFIELD(byte, loops, "Number of remaining turns or loops [0..255]", loops, )
MFIELD(float, turnR, "Current circle radius [m]", turnR, f2)
MGRP2_END
#undef MGRP2

#define MGRP2 opt
MGRP2_BEGIN("AP options")
MFIELD(bit, thrcut, "Throttle cut on/off", cmode_thrcut, )
MFIELD(bit, throvr, "Throttle override on/off", cmode_throvr, )
MFIELD(bit, ahrs, "AHRS mode inertial/gps", cmode_ahrs, )
MFIELD(bit, nomag, "Magnetometer blocked/on", cmode_nomag, )
MFIELD(bit, hover, "Stabilization mode hover/run", cmode_hover, )
MFIELD(bit, hyaw, "Hover yaw compensated/fixed", cmode_hyaw, )
MFIELD(bit, dlhd, "High precision downstream on/off", cmode_dlhd, )
MGRP2_END
#undef MGRP2

#define MGRP2 cam
MGRP2_BEGIN("Camera commanded values")
MFVEC3(att, roll, pitch, yaw)
MFIELD(float, roll, "Commanded cam roll [deg]", camcmd_roll, f2)
MFIELD(float, pitch, "Commanded cam pitch [deg]", camcmd_pitch, f2)
MFIELD(float, yaw, "Commanded cam yaw [deg]", camcmd_yaw, f2)
MFIELD(float, bias_roll, "Cam stability bias roll [deg/s]", cambias_roll, f4)
MFIELD(float, bias_pitch, "Cam stability bias pitch [deg/s]", cambias_pitch, f4)
MFIELD(float, bias_yaw, "Cam stability bias yaw [deg/s]", cambias_yaw, f4)
//MFVECT(float, att,roll,pitch,yaw,       "Commanded cam attitude [deg]",         camcmd_roll,camcmd_pitch,camcmd_yaw, f4)
//MFVECT(float, bias,roll,pitch,yaw,       "Cam stability bias [deg/s]",          cambias_roll,cambias_pitch,cambias_yaw, f4)
MFENUM(enum, camop, "Cam control mode", cam_mode, )
MFENUMV(camop, off, "camera off", 0)
MFENUMV(camop, fixed, "fixed position", 1)
MFENUMV(camop, stab, "gyro stabilization", 2)
MFENUMV(camop, position, "attitude position", 3)
MFENUMV(camop, speed, "attitude speed control", 4)
MFENUMV(camop, target, "target position tracking", 5)
MFENUM_END(camop)
MFIELD(float, zoom, "Cam zoom level [0..1]", cam_zoom, f2)
MFIELD(float, focus, "Cam focus [0..1]", cam_focus, f2)
MFIELD(uint, tperiod, "Cam period for timestamps [ms]", cam_tperiod, u2)
MFIELD(byte, ch, "Video channel [0..255]", cam_ch, )
MFIELD(byte, src, "Camera source subindex [0..255]", cam_src, )
MFIELD(bit, PF, "picture flip on/off", cam_opt_PF, )
MFIELD(bit, NIR, "NIR filter on/off", cam_opt_NIR, )
MFIELD(bit, DSP, "display information on/off", cam_opt_DSP, )
MFIELD(bit, FMI, "focus mode infinity/auto", cam_opt_FMI, )
MFIELD(bit, FM, "focus manual/auto", cam_opt_FM, )
MFIELD(bit, laser, "rangefinder on/off", cam_opt_laser, )
MFVEC3(pos, lat, lon, hmsl)
MFIELD(float, lat, "Tracking latitude [deg]", cam_lat, f4)
MFIELD(float, lon, "Tracking latitude [deg]", cam_lon, f4)
MFIELD(float, hmsl, "Tracking altitude MSL [m]", cam_hmsl, f4)
//MFVECT(float,  pos,lat,lon,hmsl,"Tracking position [deg,deg,m]",                cam_lat,cam_lon,cam_hmsl, f4)
MGRP2_END
#undef MGRP2

#define MGRP2 ats
MGRP2_BEGIN("ATS commanded values")
MFVEC3(att, roll, pitch, yaw)
MFIELD(float, roll, "Commanded ATS roll [deg]", atscmd_roll, f2)
MFIELD(float, pitch, "Commanded ATS pitch [deg]", atscmd_pitch, f2)
MFIELD(float, yaw, "Commanded ATS yaw [deg]", atscmd_yaw, f2)
//MFVECT(float, att,roll,pitch,yaw,       "Commanded ATS attitude [deg]",         atscmd_roll,atscmd_pitch,atscmd_yaw, f4)
MGRP2_END
#undef MGRP2

#define MGRP2 turret
MGRP2_BEGIN("Turret commanded values")
MFENUM(enum, turret, "Turret control mode", , )
MFENUMV(turret, off, "turret off", 0)
MFENUMV(turret, fixed, "fixed position", 1)
MFENUMV(turret, stab, "gyro stabilization", 2)
MFENUMV(turret, position, "attitude position", 3)
MFENUMV(turret, speed, "attitude speed control", 4)
MFENUM_END(turret)
MFVEC3(att, roll, pitch, yaw)
MFIELD(float, roll, "Commanded turret roll [deg]", , f2)
MFIELD(float, pitch, "Commanded turret pitch [deg]", , f2)
MFIELD(float, yaw, "Commanded turret yaw [deg]", , f2)
MFIELD(float, bias_roll, "Turret bias roll [deg/s]", , f4)
MFIELD(float, bias_pitch, "Turret bias pitch [deg/s]", , f4)
MFIELD(float, bias_yaw, "Turret bias yaw [deg/s]", , f4)
//MFVECT(float, att,roll,pitch,yaw,       "Commanded turret attitude [deg]",      ,,, f4)
//MFVECT(float, bias,roll,pitch,yaw,       "Turret stability bias [deg/s]",       ,,, f4)
MGRP2_END
#undef MGRP2

MGRP1_END
#undef MGRP1

//-----------------------------------------------------------------------------
/*#define MGRP0   pld
MGRP0_BEGIN("Payload")

MGRP0_END
#undef MGRP0

//-----------------------------------------------------------------------------
#define MGRP0   aux
MGRP0_BEGIN("Auxilary data")
#define MGRP1   local
MGRP1_BEGIN("Node local data fields")

#define MGRP2   vm
MGRP2_BEGIN("Virtual Machine accessible fields")
MGRP2_END
#undef MGRP2

#define MGRP2   tmp
MGRP2_BEGIN("Temporary data")
MGRP2_END
#undef MGRP2

#define MGRP2   storage
MGRP2_BEGIN("Persistent data")
MGRP2_END
#undef MGRP2

MGRP1_END
#undef MGRP1
//-------------------------
#define MGRP1   shared
MGRP1_BEGIN("Network shared fields")

#define MGRP2   vm
MGRP2_BEGIN("Virtual Machine accessible fields")
MGRP2_END
#undef MGRP2

#define MGRP2   tmp
MGRP2_BEGIN("Temporary data")
MGRP2_END
#undef MGRP2

MGRP1_END
#undef MGRP1

MGRP0_END
#undef MGRP0*/
//-----------------------------------------------------------------------------
/*#define MGRP0   stream
MGRP0_BEGIN("Data streams")
#define MGRP1   sid
MGRP1_BEGIN("Stream itentifier")

#define MGRP2   node
MGRP2_BEGIN("Nodes management")
MFIELD(sid, announce,  "Node annoncement message" ,, )
MFIELD(sid, search,    "Search for nodes" ,, )
MFIELD(sid, info,      "Node identity" ,, )

MFIELD(sid, fw_file,   "Init firmware update" ,, )
MFIELD(sid, fw_write,  "Write firmware data" ,, )

MFIELD(sid, reboot,    "Reboot node MCU" ,, )

MFIELD(sid, dmsg,      "Text message from MCU" ,, )

MFIELD(sid, tracert,   "Trace route" ,, )
MFIELD(sid, ping,      "Ping" ,, )
MFIELD(sid, nstat,     "Node statistics" ,, )
MFIELD(sid, mute,      "Stop sending sensors data" ,, )
MFIELD(sid, reconf,    "Reset configuration" ,, )
MFIELD(sid, mem,       "Report memory usage" ,, )
MGRP2_END
#undef MGRP2

#define MGRP2   conf
MGRP2_BEGIN("Node configuration data")
MFIELD(sid, inf,       "return _conf_inf structure" ,, )
MFIELD(sid, dsc,       "return parameter descriptor" ,, )
MFIELD(sid, cmds,      "return commands descrptor" ,, )
MFIELD(sid, read,      "return parameter" ,, )
MFIELD(sid, write,     "save parameter" ,, )
MFIELD(sid, sfile,     "get or set file info" ,, )
MFIELD(sid, sread,     "read file data" ,, )
MFIELD(sid, swrite,    "write file data" ,, )
MGRP2_END
#undef MGRP2

#define MGRP2   dict
MGRP2_BEGIN("Mandala dictionary")
MFIELD(sid, inf,       "Mandala signature and identity" ,, )
MFIELD(sid, grp,       "Group descriptor" ,, )
MFIELD(sid, fld,       "Field descriptor" ,, )
MGRP2_END
#undef MGRP2

#define MGRP2   data
MGRP2_BEGIN("Data transfer protocols")
MFIELD(sid, xpdr,      "Transponder data <xpdr>" ,, )
MFIELD(sid, ident,     "Identification <ident>" ,, )
MFIELD(sid, dlink,     "Datalink <squawk>,<mid>,<data..>" ,, )

MFIELD(sid, dstream,   "Downlink stream <stream>" ,, )
MFIELD(sid, mission,   "Mission data <packed mission>" ,, )

MFIELD(sid, active,    "Autopilot active flag" ,, )

MFIELD(sid, vmexec,    "Execute VM script <@function>" ,, )
MFIELD(sid, jsexec,    "Execute JS script <script text>" ,, )

MFIELD(sid, pld,       "Payload stream <data>" ,, )
MFIELD(sid, sim,       "Simulator data <mid>,<data..>" ,, )
MFIELD(sid, hid,       "HID data <mid>,<data..>" ,, )

MFIELD(sid, frm,       "Formation flight data <packed data>" ,, )
MGRP2_END
#undef MGRP2

#define MGRP2   usr
MGRP2_BEGIN("User data")
MFIELD(sid, vcp_local, "Serial data <port_id>,<data..>" ,, )
MFIELD(sid, vcp_lan,   "LAN Serial data <port_id>,<data..>" ,, )
MFIELD(sid, vcp_wan,   "WAN Serial data <port_id>,<data..>" ,, )
MGRP2_END
#undef MGRP2

#define MGRP2   cmd
MGRP2_BEGIN("Node specific commands")
MGRP2_END
#undef MGRP2

MGRP1_END
#undef MGRP1
MGRP0_END
#undef MGRP0*/
//=============================================================================
//=============================================================================
//=============================================================================
#undef MGRP1_BEGIN
#undef MGRP1_END
#undef MGRP2_BEGIN
#undef MGRP2_END
#undef MGRP_IMPL
#undef MFIELD
#undef MFVEC3
#undef MFVEC2
#undef MFENUM
#undef MFENUMV
#undef MFENUM_END
