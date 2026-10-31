import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog
    objectName: "extrasDialog"
    anchors.centerIn: parent
    width: Math.min(850, parent.width - 60)
    height: Math.min(640, parent.height - 80)
    modal: true
    focus: true
    title: qsTranslate("Vetust", "Extras · ") + (vetust.selected.title || "")
    footer: Item { height: 56; Button { text: qsTranslate("Vetust", "Close"); width: 100; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 12; onClicked: dialog.close() } }
    palette.window: "#111e29"; palette.windowText: "#e7edf1"
    palette.text: "#e7edf1"; palette.base: "#0b151f"
    palette.button: "#233443"; palette.buttonText: "#e7edf1"
    function begin() { search.clear(); open() }
    contentItem: ColumnLayout {
        spacing: 14
        Label {
            Layout.fillWidth: true
            text: qsTranslate("Vetust", "Maps, guides and material from the original collection. They may belong to a different edition or language.")
            color: "#a8b6c2"; wrapMode: Text.WordWrap
        }
        TextField {
            id: search; objectName: "extrasSearch"
            Layout.fillWidth: true
            placeholderText: qsTranslate("Vetust", "Search extras…")
            color: "#e7edf1"; placeholderTextColor: "#91a3b3"
        }
        ListView {
            id: files; objectName: "extrasList"
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true; spacing: 8
            model: vetust.gameExtras.filter(function(item) { return item.name.toLocaleLowerCase().indexOf(search.text.toLocaleLowerCase()) >= 0 })
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                required property var modelData
                width: files.width - 16; height: Math.max(76, content.implicitHeight + 24)
                radius: 5; color: "#172633"; border.color: "#2c414f"
                RowLayout {
                    anchors.fill: parent; anchors.margins: 12; spacing: 14
                    ColumnLayout {
                        id: content; Layout.fillWidth: true; spacing: 5
                        Label { Layout.fillWidth: true; text: modelData.name; wrapMode: Text.WrapAnywhere; color: "#e7edf1" }
                        Label { text: modelData.kind + " · " + (modelData.bytes / 1048576).toFixed(1) + " MiB"; color: "#91a3b3"; font.pixelSize: 11 }
                    }
                    Button { objectName: "openExtra" + modelData.index; text: qsTranslate("Vetust", "Open"); enabled: !vetust.busy; onClicked: vetust.openExtra(modelData.index) }
                }
            }
            Label { anchors.centerIn: parent; visible: files.count === 0; text: qsTranslate("Vetust", "No extras match that name."); color: "#91a3b3" }
        }
        Label { Layout.fillWidth: true; text: vetust.busy ? qsTranslate("Vetust", "Opening the file…") : qsTranslate("Vetust", "PDFs open here. Other formats use your system's viewer."); color: "#91a3b3"; wrapMode: Text.WordWrap; font.pixelSize: 12 }
    }
}
