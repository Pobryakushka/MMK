import QtQuick 2.0
import QtLocation 5.9

MapQuickItem {
    id: item
    anchorPoint.x: circle.width/2 * circle.scale
    anchorPoint.y: circle.height/2 * circle.scale
    property alias color: circle.color
    property string tooltipText: "tooltipText"
    property int tooltipZ: 20
//    property alias tooltipText: tooltip.text
//    property alias tooltipZ: tooltip.z
    z: 2
    sourceItem: Rectangle {
        id: circle
        color: "darkOrange"
        border.color: "red"
        border.width: 2
        opacity: 0.5
        width: 15
        height: width
        radius: height / 2
        property bool showing: false
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            onPressed: {
                if (!circle.showing) {
                    circle.showing = true;
                    Qt.createQmlObject('import QtQuick 2.0; ToolTip {id: tooltip;
                                        text: tooltipText; z: tooltipZ;
                                        Component.onDestruction: {circle.showing = false}}', circle).show(); //self-destruction
                }
            }
        }
//        ToolTip {
//            id: tooltip
//            text: "hello"
//            destroyOnHide: false
//        }
    }

}
