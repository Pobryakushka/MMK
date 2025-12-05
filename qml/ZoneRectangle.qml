import QtQuick 2.0
import QtLocation 5.9
import QtPositioning 5.8

MapItemGroup {
//    id: item
    property alias topLeft: rect.topLeft
    property alias bottomRight: rect.bottomRight
    property alias text: text.text
    property alias color: rect.color
    MapRectangle {
        id: rect
        color: "lightBlue"
        opacity: 0.25
        border.color: Qt.darker(color)
        border.width: 2
    }
    Text {
        id: text
        text: "-1"
        color: "black"//Qt.darker(rect.color)
        anchors.top: rect.top
        anchors.left: rect.left
        anchors.margins: 5
    }
}
