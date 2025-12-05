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
    function createMapComponent(pluginName) {
        console.log("createMapComponent", pluginName, map)
        var zoomLevel = 14;
        if (map != null) {
            zoomLevel = map.zoomLevel
            console.log("destroing map:", map);
            map.destroy();
        }

        map = Qt.createQmlObject('import QtQuick 2.0; import QtLocation 5.9; ' +
                                    'MapComponent { ' +
                                    'objectName: "map"; ' +
                                    'zoomLevel: ' + zoomLevel + '; ' +
                                    'plugin: Plugin { ' +
                                    'id: plugin; ' +
                                    'name: "' + pluginName + '"; ' +
                                    'PluginParameter {' +
                                    'name: "osm.mapping.host"; ' +
                                    'value: "http://a.tile.openstreetmap.org/"' +
                                    '}}}',
                                 main);

        if (map) {
             map.onSupportedMapTypesChanged.connect(function() {
                 updateMapTypes();
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
}
