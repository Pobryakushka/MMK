import QtQuick 2.15
import QtLocation 5.9
import QtPositioning 5.9
import "."


Map {
    id: map
    objectName: "map"
    anchors.fill: parent
    plugin: pluginOsm
//    center: coord.coordinateFrom
//        center: QtPositioning.coordinate(54.19609, 37.61822) // Tula
//        center: QtPositioning.coordinate(coord.latitudeFrom, coord.longitudeFrom)
    property var coordinateFrom: coord.coordinateFrom
    onCoordinateFromChanged: {
        marker.coordinate = coordinateFrom;
    }
    property var coordinateTo: coord.coordinateTo
//  onCoordinateToChanged: polyline.updateDistance()
    property bool fitView: coord.fitView
    onFitViewChanged: geoRectangle()
    onSupportedMapTypesChanged: {
        var list = new Array();
        for(var i = 0;i < map.supportedMapTypes.length; ++i) {
            list[i] = map.supportedMapTypes[i].name;
        }
        map.activeMapType = Qt.binding(function() {return map.supportedMapTypes[coord.currentMapType];})
        coord.mapTypes = list;
    }

    zoomLevel: 14

    Component.onCompleted: {
        map.center = coord.coordinateFrom;
    }

    Behavior on center {
        CoordinateAnimation {duration: 1000; easing.type:  Easing.OutCubic}
    }
    Behavior on zoomLevel {
        NumberAnimation {duration: 1000; easing.type: Easing.OutQuint}
    }

    // Запасной плагин — используется только если карта создаётся напрямую (не через Main.qml).
    // При нормальной работе плагин передаётся снаружи через createMapComponent().
    Plugin {
        id: pluginOsm
        name: "osm"
        PluginParameter { name: "osm.mapping.host"; value: "http://tile.openstreetmap.org/" }
        PluginParameter { name: "osm.useragent";    value: "MMK/1.0" }
    }

//        Timer {
//            interval: 5000; running: true; repeat: true
//            property int ii: 0
//            onTriggered: {
//                map.activeMapType = map.supportedMapTypes[ii];
//                ii++;
//                console.log(ii, map.supportedMapTypes.length);
//                if (ii >= map.supportedMapTypes.length)
//                    ii = 0;
//            }
//        }

//    Timer {
//        interval: 100; running: true; repeat: false
//        onTriggered: {
//            var list = coord.mapTypes;
//            for(var i = 0;i < map.supportedMapTypes.length; ++i) {
////                    console.log(map.supportedMapTypes[i].description)
////                    if(map.supportedMapTypes[i].style === MapType.CustomMap){
////                        map.activeMapType = map.supportedMapTypes[i];
////                    }
//                list[i] = map.supportedMapTypes[i].name;
//            }
//            map.activeMapType = Qt.binding(function() {return map.supportedMapTypes[coord.currentMapType];})
//            coord.mapTypes = list;
//        }
//    }

//        Address {
//            id:address
//            country: "russia"
//            city: coord.searchText
//            onCityChanged: geocodeModel.query = address
//        }
    GeocodeModel {
        id: geocodeModel
        plugin: pluginOsm
        query: coord.searchText
        onQueryChanged: console.log("onQueryChanged", query)
        autoUpdate: true
        onStatusChanged: console.log(geocodeModel.status, geocodeModel.errorString)
        onLocationsChanged: {
            console.log("onLocationChanged", count);
            console.log(errorString);
            var result = coord.searchResult;
            for (var i = 0; i < count; i++) {
                result[i] = get(i).address.text;
            }
            coord.searchResult = result;
            if (count >= 1) {
                coord.coordinateFrom = geocodeModel.get(0).coordinate;
            }
        }
    }

    MouseArea {
        id: mouseArea
        property variant lastCoordinate
        anchors.fill: parent
        acceptedButtons: /*Qt.LeftButton | */Qt.RightButton
        onPressed: {
            // Ставим точку только когда включён режим выбора ("Указать точку").
            // Иначе клик не трогает маркер — иначе он двигался бы, а координаты
            // в форме не менялись, что путало оператора.
            if (!coord.pickingEnabled)
                return;
            var c = map.toCoordinate(Qt.point(mouseX, mouseY));
            coord.coordinateFrom = c;
        }

//            onPressAndHold:{
////                if (Math.abs(map.pressX - mouse.x ) < map.jitterThreshold
////                        && Math.abs(map.pressY - mouse.y ) < map.jitterThreshold) {
//                    showMainMenu(lastCoordinate);
////                }
//            }
    }

    // Постановка точки тапом — для планшета (сенсорный экран). Обычная
    // MouseArea с Qt.LeftButton здесь не подходит: она захватывает
    // касание сразу на нажатии, из-за чего встроенный gesture-обработчик
    // Map (панорамирование/зум пальцем) перестаёт получать последующие
    // события и карту становится не подвинуть. TapHandler устроен иначе —
    // берёт "пассивный" захват и сам себя отменяет, если палец сдвинулся
    // дальше порога перетаскивания, поэтому панорамирование и зум
    // продолжают работать как раньше, а короткий тап без сдвига ставит
    // точку. Правая кнопка мыши (см. MouseArea выше) оставлена как есть —
    // для работы с обычной мышью на десктопе.
    TapHandler {
        id: tapHandler
        acceptedButtons: Qt.LeftButton
        onTapped: {
            // См. комментарий в MouseArea выше: тап ставит точку только в
            // активном режиме выбора, в остальное время — обычное
            // панорамирование/зум карты.
            if (!coord.pickingEnabled)
                return;
            var c = map.toCoordinate(tapHandler.point.position);
            coord.coordinateFrom = c;
        }
    }


    function geoRectangle(/*latitudeFrom, longitudeFrom, latitudeTo, longitudeTo*/) {
        if (!coord.fitView)
            return;
        var topLeftLat, topLeftLong, rightBottomLat, rightBottomLong;
        if (coord.coordinateFrom.latitude > coord.coordinateTo.latitude) {
            topLeftLat = coord.coordinateFrom.latitude;
            rightBottomLat = coord.coordinateTo.latitude;
        }
        else {
            topLeftLat = coord.coordinateTo.latitude;
            rightBottomLat = coord.coordinateFrom.latitude;
        }
        if (coord.coordinateFrom.longitude > coord.coordinateTo.longitude) {
            topLeftLong = coord.coordinateTo.longitude;
            rightBottomLong = coord.coordinateFrom.longitude;
        }
        else {
            topLeftLong = coord.coordinateFrom.longitude;
            rightBottomLong = coord.coordinateTo.longitude;
        }
        var add = 0.1;
        var region = QtPositioning.rectangle(QtPositioning.coordinate(topLeftLat+add, topLeftLong-add),
                                             QtPositioning.coordinate(rightBottomLat-add, rightBottomLong+add));
        console.log(region, region.isValid);
        if (region.isValid) {
            map.visibleRegion = region;
        }
    }

/*    MapPolyline {
        id: polyline
        z: marker.z - 1
        line.color: "#B200FF"
        line.width: 4
        opacity: 0.25
        smooth: true
        path: [coord.coordinateFrom, coord.coordinateTo]
//            Behavior on path {animation: coordAmin }
        function updateDistance() {
            var path = polyline.path;
            path[0] = coord.coordinateFrom;
            path[1] = coord.coordinateTo;
            polyline.path = path;
            map.geoRectangle();
            if (!coord.fitView)
                map.center = path[0];
            marker.coordinate = path[0];
            markerCurrentStation.visible = path[0] === path[1] ? false : true;
            markerCurrentStation.coordinate = path[1]
        }
    } */
    MapQuickItem {
        id: marker
        z: 2
        // Пока точка не выбрана оператором (coord.hasSelection == false),
        // coordinateFrom содержит захардкоженное значение по умолчанию
        // (см. конструктор QmlCoordinateProxy) — показывать маркер там не
        // нужно, это будет выглядеть как уже выбранная точка. Прячем через
        // opacity (не visible), чтобы появление было плавным, а не резким.
        opacity: coord.hasSelection ? 1 : 0
        Behavior on opacity {NumberAnimation {duration: 200}}
        anchorPoint.x: markerImage.width/2 * markerImage.scale
        anchorPoint.y: markerImage.height * markerImage.scale
        coordinate: coord.coordinateFrom
        sourceItem:  Image {
            id: markerImage
            source: "qrc:///dat/images/marker.png"
            scale: 0.75
        }
        Behavior on coordinate {CoordinateAnimation {duration: 1000; easing.type:  Easing.OutCubic}}
//            MouseArea {
//                anchors.fill: parent
//                acceptedButtons: Qt.RightButton
//                onPressed: {
//                    console.log(polyline.path[0]);
//                    geocodeModel.query = "Екатеринбург"//polyline.path[0];
//                    geocodeModel.update();
////                    console.log(geocodeModel.status);
////                    timer.start();
//                }
//            }
//            Timer {
//                id: timer
//                interval: 1000
//                repeat: true
//                running: false
//                onTriggered: console.log(geocodeModel.status, geocodeModel.errorString)
//            }
    }
//    MarkerStation {
//        id: markerCurrentStation
//        color: "black"
//    }
    function createStationsMarkers(coordinate, tooltipText) {
        var component = Qt.createComponent("MarkerStation.qml");
        var item = component.createObject(map, {"coordinate": coordinate, "tooltipText": tooltipText, "tooltipZ": map.z + 1});
        map.addMapItem(map.addMapItem(item));
    }

    function createZoneRect(topLeft, bottomRight, zoneNumber, color) {
        var component = Qt.createComponent("ZoneRectangle.qml");
        var item = component.createObject(map, {"topLeft": topLeft, "bottomRight": bottomRight,
                                              "text": zoneNumber, "color": color});
//            var item = component.createObject(map);
//            item.topLeft = topLeft;
//            item.bottomRight = bottomRight;           //можно так
//            item.text = zoneNumber;
//            item.color = color;
        if (item) {
            map.addMapItemGroup(item);
            return item;
        }
        else
            return null;
    }

    function createColdZoneRect(topLeft, bottomRight, zoneNumber, color) {
        var item = createZoneRect(topLeft, bottomRight, zoneNumber, color);
        item.visible = Qt.binding(function() {return coord.coldZonesVisible;});
    }

    function createWarmZoneRect(topLeft, bottomRight, zoneNumber, color) {
        var item = createZoneRect(topLeft, bottomRight, zoneNumber, color);
        item.visible = Qt.binding(function() {return coord.warmZonesVisible;});
    }

//        ZoneRectangle {
//            topLeft: QtPositioning.coordinate(50, 30);
//            bottomRight: QtPositioning.coordinate(73, 90);
//        }

}
