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

#include <QByteArray>
#include <QFile>
#include <QHash>
#include <QImage>
#include <QList>
#include <QString>
#include <QVector>

#include <cstdint>
#include <memory>

/**
 * One ASTER GDEM (GeoTIFF, 16-bit, one sample per pixel) elevation tile.
 *
 * The file is memory mapped and decoded lazily, one TIFF tile/strip
 * ("block") at a time, so only the blocks actually touched by requests
 * are ever decompressed and kept in memory (128 KiB per 256x256 block
 * instead of ~26 MiB for a whole 3601x3601 tile).
 *
 * Supported layouts: classic TIFF (little or big endian), 16-bit samples,
 * tiled or stripped, compression none (1) or LZW (5), predictor 1 or 2.
 * Anything else falls back to a full QImage decode of the file.
 *
 * Not thread-safe: an instance must be used from one thread only.
 */
class ElevationTile
{
public:
    static constexpr int MAX_CACHED_BLOCKS = 128; // per tile (256x256x2 bytes each => 16 MiB max)

    // Returns nullptr when the file does not exist or can't be read at all
    static std::unique_ptr<ElevationTile> open(const QString &filePath);

    ~ElevationTile();

    const QString &filePath() const { return m_filePath; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    bool isValid() const { return m_width > 0 && m_height > 0; }
    bool isFallback() const { return !m_fallback.isNull(); }
    int cachedBlocks() const { return m_blocks.size(); }
    qsizetype cachedBytes() const { return m_blocks.size() * qsizetype(m_blockW) * m_blockH * 2; }

    // Elevation at geographic position (fractional part of lat/lon selects the pixel).
    // Returns NaN when the sample can't be read.
    double elevationAt(double lat, double lon);

    // Raw sample at pixel; returns false when the sample can't be read.
    bool sample(int x, int y, int16_t &value);

    // Ground size of one pixel at the latitude [m]: the smaller of the
    // north-south and east-west spacing (a 1 degree tile is assumed).
    double resolution(double lat) const;

private:
    ElevationTile() = default;
    bool parse();
    const int16_t *block(int index);
    bool decodeBlock(int index, QByteArray &out);
    static bool decodeLZW(const uint8_t *src, qsizetype srcLen, uint8_t *dst, qsizetype dstLen);
    void touch(int index);

    QString m_filePath;
    QFile m_file;
    const uint8_t *m_data{nullptr};
    qint64 m_size{0};
    bool m_bigEndian{false};

    int m_width{0};
    int m_height{0};
    int m_compression{1};
    int m_predictor{1};
    bool m_signedSamples{true};

    int m_blockW{0};
    int m_blockH{0};
    int m_blocksAcross{0};
    QVector<quint64> m_offsets;
    QVector<quint64> m_byteCounts;

    QHash<int, QByteArray> m_blocks; // decoded blocks, native endian int16
    QList<int> m_lru;                // block indices, most recently used last

    QImage m_fallback; // whole image decoded by Qt when the layout is not supported
};
