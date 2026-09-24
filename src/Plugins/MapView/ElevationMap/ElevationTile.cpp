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
#include "ElevationTile.h"

#include <QtEndian>

#include <cmath>
#include <cstring>

namespace {

// TIFF tags used by the reader
enum Tag : uint16_t {
    ImageWidth = 256,
    ImageLength = 257,
    BitsPerSample = 258,
    Compression = 259,
    StripOffsets = 273,
    SamplesPerPixel = 277,
    RowsPerStrip = 278,
    StripByteCounts = 279,
    PlanarConfig = 284,
    Predictor = 317,
    TileWidth = 322,
    TileLength = 323,
    TileOffsets = 324,
    TileByteCounts = 325,
    SampleFormat = 339,
};

enum Type : uint16_t {
    TypeByte = 1,
    TypeShort = 3,
    TypeLong = 4,
};

struct Reader
{
    const uint8_t *data;
    qint64 size;
    bool be;

    bool inRange(qint64 off, qint64 len) const { return off >= 0 && len >= 0 && off + len <= size; }
    uint16_t u16(qint64 off) const
    {
        return be ? qFromBigEndian<quint16>(data + off) : qFromLittleEndian<quint16>(data + off);
    }
    uint32_t u32(qint64 off) const
    {
        return be ? qFromBigEndian<quint32>(data + off) : qFromLittleEndian<quint32>(data + off);
    }
};

} // namespace

std::unique_ptr<ElevationTile> ElevationTile::open(const QString &filePath)
{
    std::unique_ptr<ElevationTile> tile(new ElevationTile());
    tile->m_filePath = filePath;
    tile->m_file.setFileName(filePath);
    if (!tile->m_file.open(QIODevice::ReadOnly))
        return nullptr;

    tile->m_size = tile->m_file.size();
    if (tile->m_size > 16) {
        tile->m_data = tile->m_file.map(0, tile->m_size);
    }

    if (tile->m_data && tile->parse())
        return tile;

    // Unsupported layout - let Qt decode the whole file
    tile->m_file.close();
    tile->m_data = nullptr;
    tile->m_fallback = QImage(filePath);
    if (tile->m_fallback.isNull())
        return nullptr;
    if (tile->m_fallback.format() != QImage::Format_Grayscale16)
        tile->m_fallback = tile->m_fallback.convertToFormat(QImage::Format_Grayscale16);
    if (tile->m_fallback.isNull())
        return nullptr;
    tile->m_width = tile->m_fallback.width();
    tile->m_height = tile->m_fallback.height();
    return tile;
}

ElevationTile::~ElevationTile() = default;

bool ElevationTile::parse()
{
    Reader r{m_data, m_size, false};
    if (m_data[0] == 'I' && m_data[1] == 'I')
        r.be = false;
    else if (m_data[0] == 'M' && m_data[1] == 'M')
        r.be = true;
    else
        return false;
    m_bigEndian = r.be;

    if (r.u16(2) != 42) // classic TIFF only (BigTIFF is 43)
        return false;

    const qint64 ifd = r.u32(4);
    if (!r.inRange(ifd, 2))
        return false;
    const int entries = r.u16(ifd);
    if (!r.inRange(ifd + 2, qint64(entries) * 12))
        return false;

    int bits = 0, samples = 1, planar = 1, sampleFormat = 2;
    int rowsPerStrip = 0, tileW = 0, tileH = 0;
    QVector<quint64> stripOffsets, stripCounts, tileOffsets, tileCounts;

    auto readArray = [&](qint64 entry, uint16_t type, uint32_t count, QVector<quint64> &out) {
        const int esz = (type == TypeShort)  ? 2
                        : (type == TypeLong) ? 4
                        : (type == TypeByte) ? 1
                                             : 0;
        if (!esz)
            return false;
        const qint64 total = qint64(esz) * count;
        const qint64 off = total <= 4 ? entry + 8 : r.u32(entry + 8);
        if (!r.inRange(off, total))
            return false;
        out.resize(count);
        for (uint32_t i = 0; i < count; ++i) {
            const qint64 p = off + qint64(i) * esz;
            out[i] = esz == 1 ? m_data[p] : esz == 2 ? r.u16(p) : r.u32(p);
        }
        return true;
    };
    auto readScalar = [&](qint64 entry, uint16_t type, uint32_t count, int &out) {
        QVector<quint64> v;
        if (count < 1 || !readArray(entry, type, count, v))
            return false;
        out = int(v[0]);
        return true;
    };

    for (int i = 0; i < entries; ++i) {
        const qint64 e = ifd + 2 + qint64(i) * 12;
        const uint16_t tag = r.u16(e);
        const uint16_t type = r.u16(e + 2);
        const uint32_t count = r.u32(e + 4);
        bool ok = true;
        switch (tag) {
        case ImageWidth:
            ok = readScalar(e, type, count, m_width);
            break;
        case ImageLength:
            ok = readScalar(e, type, count, m_height);
            break;
        case BitsPerSample:
            ok = readScalar(e, type, count, bits);
            break;
        case Compression:
            ok = readScalar(e, type, count, m_compression);
            break;
        case SamplesPerPixel:
            ok = readScalar(e, type, count, samples);
            break;
        case RowsPerStrip:
            ok = readScalar(e, type, count, rowsPerStrip);
            break;
        case PlanarConfig:
            ok = readScalar(e, type, count, planar);
            break;
        case Predictor:
            ok = readScalar(e, type, count, m_predictor);
            break;
        case SampleFormat:
            ok = readScalar(e, type, count, sampleFormat);
            break;
        case TileWidth:
            ok = readScalar(e, type, count, tileW);
            break;
        case TileLength:
            ok = readScalar(e, type, count, tileH);
            break;
        case StripOffsets:
            ok = readArray(e, type, count, stripOffsets);
            break;
        case StripByteCounts:
            ok = readArray(e, type, count, stripCounts);
            break;
        case TileOffsets:
            ok = readArray(e, type, count, tileOffsets);
            break;
        case TileByteCounts:
            ok = readArray(e, type, count, tileCounts);
            break;
        default:
            break;
        }
        if (!ok)
            return false;
    }

    if (m_width <= 0 || m_height <= 0 || bits != 16 || samples != 1 || planar != 1)
        return false;
    if (m_compression != 1 && m_compression != 5)
        return false;
    if (m_predictor != 1 && m_predictor != 2)
        return false;
    if (sampleFormat != 1 && sampleFormat != 2)
        return false;
    m_signedSamples = sampleFormat == 2;

    if (!tileOffsets.isEmpty()) {
        if (tileW <= 0 || tileH <= 0 || (tileW % 16) || tileOffsets.size() != tileCounts.size())
            return false;
        m_blockW = tileW;
        m_blockH = tileH;
        m_blocksAcross = (m_width + tileW - 1) / tileW;
        const int blocksDown = (m_height + tileH - 1) / tileH;
        if (tileOffsets.size() < qsizetype(m_blocksAcross) * blocksDown)
            return false;
        m_offsets = tileOffsets;
        m_byteCounts = tileCounts;
    } else if (!stripOffsets.isEmpty()) {
        if (rowsPerStrip <= 0 || rowsPerStrip > m_height)
            rowsPerStrip = m_height;
        if (stripOffsets.size() != stripCounts.size())
            return false;
        m_blockW = m_width;
        m_blockH = rowsPerStrip;
        m_blocksAcross = 1;
        const int blocksDown = (m_height + rowsPerStrip - 1) / rowsPerStrip;
        if (stripOffsets.size() < blocksDown)
            return false;
        m_offsets = stripOffsets;
        m_byteCounts = stripCounts;
    } else {
        return false;
    }

    for (qsizetype i = 0; i < m_offsets.size(); ++i) {
        if (!r.inRange(qint64(m_offsets[i]), qint64(m_byteCounts[i])))
            return false;
    }
    return true;
}

double ElevationTile::elevationAt(double lat, double lon)
{
    if (!isValid())
        return NAN;

    double temp;
    auto modY = std::modf(lat, &temp);
    auto modX = std::modf(lon, &temp);
    if (modY < 0)
        modY++;
    if (modX < 0)
        modX++;

    int pixelY = static_cast<int>(0.5 + std::abs((m_height - 1) * (1 - modY)));
    int pixelX = static_cast<int>(0.5 + std::abs((m_width - 1) * modX));
    if (pixelY >= m_height)
        pixelY = m_height - 1;
    if (pixelX >= m_width)
        pixelX = m_width - 1;

    int16_t v;
    if (!sample(pixelX, pixelY, v))
        return NAN;
    return static_cast<double>(v);
}

bool ElevationTile::sample(int x, int y, int16_t &value)
{
    if (x < 0 || y < 0 || x >= m_width || y >= m_height)
        return false;

    if (!m_fallback.isNull()) {
        const uchar *line = m_fallback.constScanLine(y);
        if (!line)
            return false;
        int16_t v;
        std::memcpy(&v, line + qsizetype(x) * 2, sizeof(v));
        value = v;
        return true;
    }

    const int index = (y / m_blockH) * m_blocksAcross + (x / m_blockW);
    const int16_t *b = block(index);
    if (!b)
        return false;
    value = b[(y % m_blockH) * m_blockW + (x % m_blockW)];
    return true;
}

const int16_t *ElevationTile::block(int index)
{
    auto it = m_blocks.find(index);
    if (it != m_blocks.end()) {
        touch(index);
        return reinterpret_cast<const int16_t *>(it->constData());
    }

    QByteArray decoded;
    if (!decodeBlock(index, decoded))
        return nullptr;

    while (m_blocks.size() >= MAX_CACHED_BLOCKS && !m_lru.isEmpty()) {
        m_blocks.remove(m_lru.takeFirst());
    }
    it = m_blocks.insert(index, decoded);
    m_lru.append(index);
    return reinterpret_cast<const int16_t *>(it->constData());
}

void ElevationTile::touch(int index)
{
    if (m_lru.isEmpty() || m_lru.last() == index)
        return;
    m_lru.removeOne(index);
    m_lru.append(index);
}

bool ElevationTile::decodeBlock(int index, QByteArray &out)
{
    if (index < 0 || index >= m_offsets.size())
        return false;

    const qsizetype blockBytes = qsizetype(m_blockW) * m_blockH * 2;
    out.resize(blockBytes);
    uint8_t *dst = reinterpret_cast<uint8_t *>(out.data());
    const uint8_t *src = m_data + m_offsets[index];
    const qsizetype srcLen = qsizetype(m_byteCounts[index]);

    // the last strip may be shorter than RowsPerStrip
    qsizetype need = blockBytes;
    if (m_blocksAcross == 1) {
        const int rows = qMin(m_blockH, m_height - (index / m_blocksAcross) * m_blockH);
        need = qsizetype(rows) * m_blockW * 2;
    }

    if (m_compression == 1) {
        if (srcLen < need)
            return false;
        std::memcpy(dst, src, need);
    } else {
        std::memset(dst, 0, blockBytes);
        if (!decodeLZW(src, srcLen, dst, need))
            return false;
    }

    int16_t *samples = reinterpret_cast<int16_t *>(dst);
    const qsizetype count = need / 2;
    if (m_bigEndian != (Q_BYTE_ORDER == Q_BIG_ENDIAN)) {
        for (qsizetype i = 0; i < count; ++i)
            samples[i] = int16_t(qbswap(uint16_t(samples[i])));
    }
    if (m_predictor == 2) {
        const qsizetype rows = count / m_blockW;
        for (qsizetype row = 0; row < rows; ++row) {
            uint16_t *line = reinterpret_cast<uint16_t *>(samples + row * m_blockW);
            for (int x = 1; x < m_blockW; ++x)
                line[x] = uint16_t(line[x] + line[x - 1]);
        }
    }
    return true;
}

// TIFF 6.0 LZW (MSB-first codes, 9..12 bits, "early change")
bool ElevationTile::decodeLZW(const uint8_t *src, qsizetype srcLen, uint8_t *dst, qsizetype dstLen)
{
    constexpr int ClearCode = 256;
    constexpr int EoiCode = 257;
    constexpr int MaxCodes = 4096;

    static thread_local QVector<int> prefix;
    static thread_local QVector<uint8_t> suffix;
    static thread_local QVector<int> length;
    if (prefix.size() != MaxCodes) {
        prefix.resize(MaxCodes);
        suffix.resize(MaxCodes);
        length.resize(MaxCodes);
        for (int i = 0; i < 256; ++i) {
            prefix[i] = -1;
            suffix[i] = uint8_t(i);
            length[i] = 1;
        }
    }

    qsizetype outPos = 0;
    qsizetype bitPos = 0;
    const qsizetype bitLen = srcLen * 8;
    int codeLen = 9;
    int nextCode = 258;
    int prev = -1;

    auto readCode = [&](int &code) {
        if (bitPos + codeLen > bitLen)
            return false;
        uint32_t v = 0;
        qsizetype byte = bitPos >> 3;
        // read 3 bytes (enough for 12 bits at any alignment)
        for (int i = 0; i < 3; ++i) {
            v <<= 8;
            if (byte + i < srcLen)
                v |= src[byte + i];
        }
        const int shift = 24 - int(bitPos & 7) - codeLen;
        code = int((v >> shift) & ((1u << codeLen) - 1));
        bitPos += codeLen;
        return true;
    };

    auto firstChar = [&](int code) {
        while (prefix[code] >= 0)
            code = prefix[code];
        return suffix[code];
    };

    // writes the string of `code` at dst[outPos], returns false when out of space
    auto writeString = [&](int code) {
        const int len = length[code];
        if (outPos + len > dstLen)
            return false;
        uint8_t *p = dst + outPos + len;
        int c = code;
        while (c >= 0) {
            *--p = suffix[c];
            c = prefix[c];
        }
        outPos += len;
        return true;
    };

    auto addEntry = [&](int prefixCode, uint8_t ch) {
        if (nextCode >= MaxCodes)
            return;
        prefix[nextCode] = prefixCode;
        suffix[nextCode] = ch;
        length[nextCode] = length[prefixCode] + 1;
        nextCode++;
        if (nextCode + 1 >= (1 << codeLen) && codeLen < 12)
            codeLen++;
    };

    int code;
    while (outPos < dstLen && readCode(code)) {
        if (code == ClearCode) {
            codeLen = 9;
            nextCode = 258;
            prev = -1;
            continue;
        }
        if (code == EoiCode)
            break;

        if (prev < 0) {
            if (code >= 256)
                return false;
            if (!writeString(code))
                return false;
            prev = code;
            continue;
        }

        if (code < nextCode) {
            if (!writeString(code))
                return false;
            addEntry(prev, firstChar(code));
        } else if (code == nextCode) {
            // KwKwK case: string(prev) + firstChar(prev)
            const uint8_t ch = firstChar(prev);
            addEntry(prev, ch);
            if (!writeString(code))
                return false;
        } else {
            return false; // corrupt stream
        }
        prev = code;
    }
    return outPos >= dstLen;
}
