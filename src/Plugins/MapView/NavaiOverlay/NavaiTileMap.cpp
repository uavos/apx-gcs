#include "NavaiTileMap.h"

#include <cmath>

namespace {
// A bucket is a square of this many degrees; about 5.5 km, 20 x 20 tiles of 250 m.
constexpr double kBucketDeg = 0.05;
} // namespace

int NavaiTileMap::bucketRow(double latitude)
{
    return static_cast<int>(std::floor(latitude / kBucketDeg));
}

int NavaiTileMap::bucketColumn(double longitude)
{
    return static_cast<int>(std::floor(longitude / kBucketDeg));
}

qint64 NavaiTileMap::bucketKey(int row, int column)
{
    return (static_cast<qint64>(row) << 32) | static_cast<quint32>(column);
}

void NavaiTileMap::rebuildIndex()
{
    _rows.clear();
    _buckets.clear();
    _scored = 0;
    for (int index = 0; index < _tiles.size(); ++index) {
        const NavaiTile &tile = _tiles[index];
        _rows.insert(tile.id, index);
        _buckets[bucketKey(bucketRow(tile.latitude), bucketColumn(tile.longitude))].append(index);
        if (tile.score >= 0.0f)
            _scored++;
    }
}

void NavaiTileMap::replace(const QVector<NavaiTile> &tiles)
{
    // NAVAI resends the whole grid during the flight; a tile that is already
    // known keeps its score, only the geometry is taken from the packet.
    QVector<NavaiTile> items = tiles;
    for (NavaiTile &tile : items) {
        const int index = _rows.value(tile.id, -1);
        if (index >= 0)
            tile.score = _tiles[index].score;
    }
    _tiles = items;
    rebuildIndex();
}

void NavaiTileMap::merge(const QVector<NavaiTile> &tiles)
{
    // New tiles are added to the grid, known ones are refreshed in place.
    QVector<NavaiTile> merged = _tiles;
    for (const NavaiTile &tile : tiles) {
        const int index = _rows.value(tile.id, -1);
        if (index < 0)
            merged.append(tile);
        else
            merged[index] = tile;
    }
    replace(merged);
}

bool NavaiTileMap::setScore(const QString &id, float score)
{
    const int index = _rows.value(id, -1);
    if (index < 0)
        return false;
    NavaiTile &tile = _tiles[index];
    if (tile.score < 0.0f && score >= 0.0f)
        _scored++;
    else if (tile.score >= 0.0f && score < 0.0f)
        _scored--;
    tile.score = score;
    return true;
}

void NavaiTileMap::clear()
{
    _tiles.clear();
    rebuildIndex();
}

QVector<int> NavaiTileMap::inside(double south, double north, double west, double east) const
{
    QVector<int> result;
    if (_tiles.isEmpty())
        return result;

    const int firstRow = bucketRow(south) - 1;
    const int lastRow = bucketRow(north) + 1;
    const int firstColumn = bucketColumn(west) - 1;
    const int lastColumn = bucketColumn(east) + 1;

    const qint64 span = static_cast<qint64>(lastRow - firstRow + 1)
                        * static_cast<qint64>(lastColumn - firstColumn + 1);
    if (span > _buckets.size()) {
        // The view is wider than the map: walking the buckets we have is cheaper.
        for (auto it = _buckets.constBegin(); it != _buckets.constEnd(); ++it) {
            const int row = static_cast<int>(it.key() >> 32);
            const int column = static_cast<int>(it.key() & 0xffffffff);
            if (row < firstRow || row > lastRow || column < firstColumn || column > lastColumn)
                continue;
            result += it.value();
        }
        return result;
    }

    for (int row = firstRow; row <= lastRow; ++row) {
        for (int column = firstColumn; column <= lastColumn; ++column) {
            const auto it = _buckets.constFind(bucketKey(row, column));
            if (it != _buckets.constEnd())
                result += it.value();
        }
    }
    return result;
}
