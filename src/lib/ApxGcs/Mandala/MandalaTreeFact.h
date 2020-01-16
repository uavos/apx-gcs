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
#ifndef MandalaTreeFact_H
#define MandalaTreeFact_H
//=============================================================================
#include <Fact/Fact.h>
#include <Mandala/MandalaMetaBase.h>
#include <Mandala/MandalaStream.h>
#include <QtCore>

class MandalaTree;

class MandalaTreeFact : public Fact
{
    Q_OBJECT
public:
    explicit MandalaTreeFact(MandalaTree *tree, Fact *parent, const mandala::meta_t &meta);

    bool setValue(const QVariant &v); //override
    bool setValueLocal(const QVariant &v);

    Q_INVOKABLE quint16 id();
    Q_INVOKABLE void request();
    Q_INVOKABLE void send();

    size_t pack(void *buf) const;
    size_t unpack(const void *buf);

private:
    MandalaTree *m_tree;
    const mandala::meta_t &m_meta;
    QElapsedTimer sendTime;
    QTimer sendTimer;

    class stream_base_t
    {
    public:
        virtual size_t pack(void *buf, const QVariant &v) const = 0;
        virtual size_t unpack(const void *buf, QVariant &v) = 0;
        virtual size_t psize() const = 0;
    };

    template<mandala::sfmt_id_t _sfmt, typename _DataType>
    class stream_t : public stream_base_t
    {
    protected:
        size_t pack(void *buf, const QVariant &v) const override
        {
            return mandala::stream::pack<_sfmt, _DataType>(buf, v.value<_DataType>());
        }
        size_t unpack(const void *buf, QVariant &v) override
        {
            _DataType d;
            size_t sz = mandala::stream::unpack<_sfmt, _DataType>(buf, d);
            if (sz > 0)
                v = QVariant::fromValue(d);
            return sz;
        }
        size_t psize() const override { return mandala::stream::psize<_sfmt>(); }
    };

    stream_base_t *m_stream{nullptr};
    stream_base_t *get_stream();

    template<mandala::sfmt_id_t _sfmt>
    stream_base_t *get_stream(mandala::type_id_t type)
    {
        switch (type) {
        case mandala::type_float:
            type_text = "float";
            return new stream_t<_sfmt, mandala::float_t>();
        case mandala::type_dword:
            type_text = "dword";
            return new stream_t<_sfmt, mandala::dword_t>();
        case mandala::type_word:
            type_text = "word";
            return new stream_t<_sfmt, mandala::word_t>();
        case mandala::type_byte:
            type_text = "byte";
            return new stream_t<mandala::sfmt_u1, mandala::byte_t>();
        case mandala::type_enum:
            type_text = "enum";
            return new stream_t<mandala::sfmt_u1, mandala::enum_t>();
        }
    }

    QString sfmt_text;
    QString type_text;

protected:
    //Fact override
    virtual QVariant data(int col, int role) const;
    virtual bool showThis(QRegExp re) const; //filter helper

private slots:
    void updateStatus();
    void updateDescr();

signals:
    void sendValueUpdate(quint16 id, double v);
    void sendValueRequest(quint16 id);
};
//=============================================================================
#endif
