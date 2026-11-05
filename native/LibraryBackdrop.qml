import QtQuick
import QtQuick.Window
import QtQuick.Effects

Item {
    id: backdrop
    property url source
    property color baseColor: "#253443"
    readonly property bool software: GraphicsInfo.api === GraphicsInfo.Software
    clip: true
    Accessible.ignored: true

    Rectangle { anchors.fill: parent; color: backdrop.baseColor }
    Image {
        id: artwork
        objectName: "libraryBackgroundImage"
        anchors.fill: parent; anchors.margins: -24
        source: backdrop.source
        sourceSize.width: backdrop.software ? 64 : 960
        fillMode: Image.PreserveAspectCrop
        smooth: true; asynchronous: true
        // La ampliación suavizada mantiene el fondo difuminado también sin aceleración gráfica.
        visible: backdrop.software
        opacity: backdrop.software ? 0.72 : 1
    }
    MultiEffect {
        anchors.fill: artwork
        source: artwork
        visible: !backdrop.software && artwork.status === Image.Ready
        autoPaddingEnabled: false
        blurEnabled: true; blurMax: 48; blur: 0.85
        saturation: -0.12
        opacity: 0.72
    }
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "#a608101a" }
            GradientStop { position: 0.28; color: "#7008101a" }
            GradientStop { position: 0.72; color: "#8508101a" }
            GradientStop { position: 1; color: "#c008101a" }
        }
    }
}
