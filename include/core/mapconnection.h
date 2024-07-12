#pragma once
#ifndef MAPCONNECTION_H
#define MAPCONNECTION_H

#include <QString>
#include <QHash>

class Map;

class MapConnection {
public:
    QString direction;
    int offset;
    QString map_name;

    static const QStringList cardinalDirections;
    static bool isCardinal(const QString &direction);
    static bool isHorizontal(const QString &direction);
    static bool isVertical(const QString &direction);
    static MapConnection mirror(const MapConnection &source, const QString &mapName);
};

struct MapConnectionMirror {
    MapConnection * connection = nullptr;
    Map * map = nullptr;
};

inline bool operator==(const MapConnection &c1, const MapConnection &c2) {
    return c1.direction == c2.direction &&
           c1.offset == c2.offset &&
           c1.map_name == c2.map_name;
}

#endif // MAPCONNECTION_H
