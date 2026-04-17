import QtQuick 2.0
import QtLocation 5.9
//import QtLocation 5.12
import "."

Item {
    id: main
    property var map: null
//    MapComponent {
//        id: currentMap
//        plugin: plugin
//        Plugin {
//            id: plugin
//            name: "osm"
//        }
//    }
//    Component.onCompleted: {createMapComponent("osm")}
    property var gnssMarker: null

    Component {
        id: markerComponent
        MapQuickItem {
            anchorPoint.x: sourceItem.width / 2
            anchorPoint.y: sourceItem.height
            sourceItem: Rectangle {
                width: 20
                height: 20
                radius: 10
                color: "#2196F3" // Синий
                border.color: "white"
                border.width: 2
            }
        }
    }
    function createMapComponent(pluginName) {
        console.log("createMapComponent", pluginName, map)
        var zoomLevel = 14;
        if (map != null) {
            zoomLevel = map.zoomLevel
            console.log("destroing map:", map);
            map.destroy();
        }

        // Пути и URL передаются из C++ как context properties (только ASCII —
        // нелатинские символы в строках Qt.createQmlObject ломают парсер QML).
        var cacheParam     = (typeof mapCacheDir     !== "undefined" && mapCacheDir     !== "")
            ? 'PluginParameter { name: "osm.mapping.cache.directory"; value: "' + mapCacheDir + '" } '
            : '';
        // Локальный JSON-файл провайдеров: заменяет Thunderforest (платный) на OSM-тайлы
        var providersParam = (typeof osmProvidersUrl !== "undefined" && osmProvidersUrl !== "")
            ? 'PluginParameter { name: "osm.mapping.providersrepository.address"; value: "' + osmProvidersUrl + '" } '
            : '';

        map = Qt.createQmlObject('import QtQuick 2.0; import QtLocation 5.9; ' +
                                    'MapComponent { ' +
                                    'objectName: "map"; ' +
                                    'zoomLevel: ' + zoomLevel + '; ' +
                                    'plugin: Plugin { ' +
                                    'id: plugin; ' +
                                    'name: "osm"; ' +
                                    'PluginParameter { name: "osm.useragent"; value: "MMK/1.0" } ' +
                                    providersParam +
                                    cacheParam +
                                    '}}',
                                 main);

        if (map) {
            map.onSupportedMapTypesChanged.connect(function() {
                updateMapTypes();
            });
            map.onVisibleRegionChanged.connect(function() {
                var r = map.visibleRegion;
                if (r && r.isValid) {
                    coord.visibleNorth = r.topLeft.latitude;
                    coord.visibleSouth = r.bottomRight.latitude;
                    coord.visibleWest  = r.topLeft.longitude;
                    coord.visibleEast  = r.bottomRight.longitude;
                }
            });
            updateMapTypes();
        }
    }
    function updateMapTypes() {
        if (map && map.supportedMapTypes && map.supportedMapTypes.length > 0) {
            console.log("Map types available:", map.supportedMapTypes.length);
            var mapTypes = [];
            for (var i = 0; i < map.supportedMapTypes.length; i++) {
                mapTypes.push(map.supportedMapTypes[i].name);
            }
            coord.mapTypes = mapTypes;
        } else {
            console.log("Map types not yet available");
        }
    }

    Connections {
        target: coord
        onCurrentMapTypeChanged: {
            if (map && map.supportedMapTypes && map.supportedMapTypes.length > coord.currentMapType) {
                map.activeMapType = map.supportedMapTypes[coord.currentMapType];
            }
        }
    }

    // Возвращает видимые границы карты как объект {north, south, west, east}.
    // Вызывается из C++ через QMetaObject::invokeMethod с Q_RETURN_ARG(QVariant).
    function getVisibleBounds() {
        if (!map) return {"north": 0, "south": 0, "west": 0, "east": 0};
        var r = map.visibleRegion;
        if (!r.isValid) return {"north": 0, "south": 0, "west": 0, "east": 0};
        var tl = r.topLeft;
        var br = r.bottomRight;
        return {
            "north": tl.latitude,
            "south": br.latitude,
            "west":  tl.longitude,
            "east":  br.longitude
        };
    }

    function createStationsMarkers(coordinate, tooltipText) {
        if (map != null)
            map.createStationsMarkers(coordinate, tooltipText);
    }

    function createColdZoneRect(topLeft, bottomRight, zoneNumber, color) {
        if (map != null)
            map.createColdZoneRect(topLeft, bottomRight, zoneNumber, color);
    }

    function createWarmZoneRect(topLeft, bottomRight, zoneNumber, color) {
        if (map != null)
            map.createWarmZoneRect(topLeft, bottomRight, zoneNumber, color);
    }

    function updateGnssMarker(lat, lon, enabled) {
        if (enabled) {
            // Создаем или обновляем маркер GNSS
            if (!gnssMarker) {
                gnssMarker = markerComponent.createObject(map, {
                    coordinate: QtPositioning.coordinate(lat, lon)
                                                          })
            } else {
                gnssMarker.coordinate = QtPositioning.coordinate(lat, lon)
            }
            gnssMarker.visible = true
        } else {
            if (gnssMarker) {
                gnssMarker.visible = false
            }
        }
    }
}
