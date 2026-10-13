#pragma once

#include <QHash>
#include <QString>
#include <QVector>

struct NavaiTile
{
    QString id;
    double latitude = 0.0;  // tile centre
    double longitude = 0.0;
    double sizeM = 0.0;     // side of the square tile
    float score = -1.0f;    // -1 means the tile was never scored
};

// The copy of the NAVAI map kept on the ground station: every tile with its
// last score. Tiles are bucketed by position so drawing a map view touches
// only the tiles under it, however big the map is.
class NavaiTileMap
{
public:
    void replace(const QVector<NavaiTile> &tiles);
    void merge(const QVector<NavaiTile> &tiles);
    bool setScore(const QString &id, float score);
    void clear();

    int count() const { return _tiles.size(); }
    int scoredCount() const { return _scored; }
    const NavaiTile &tile(int index) const { return _tiles[index]; }

    // Indices of the tiles whose centre lies inside the bounds, with one
    // bucket of margin so tiles crossing the edge are included.
    QVector<int> inside(double south, double north, double west, double east) const;

private:
    void rebuildIndex();
    static qint64 bucketKey(int row, int column);
    static int bucketRow(double latitude);
    static int bucketColumn(double longitude);

    QVector<NavaiTile> _tiles;
    QHash<QString, int> _rows;           // tile id -> index in _tiles
    QHash<qint64, QVector<int>> _buckets; // bucket -> indices in _tiles
    int _scored = 0;
};
