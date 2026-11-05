import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs

Dialog {
    id: dialog
    objectName: "backgroundsDialog"
    required property var sections
    required property url previewSource
    readonly property string section: sectionPicker.currentValue || "home"
    property string error: ""

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 32, 840)
    height: Math.min(parent.height - 32, 590)
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    palette.window: "#111e29"; palette.windowText: "#e7edf1"
    palette.text: "#e7edf1"; palette.button: "#233443"; palette.buttonText: "#e7edf1"

    function begin(currentSection) {
        sectionPicker.currentIndex=Math.max(0, sections.findIndex(s => s.value === currentSection))
        error=""
        open()
    }
    header: RowLayout {
        Label { Layout.fillWidth: true; Layout.margins: 12; font.bold: true; text: qsTranslate("Vetust", "Section backgrounds") }
        ToolButton {
            Layout.rightMargin: 8; text: "×"; font.pixelSize: 26
            Accessible.name: qsTranslate("Vetust", "Close")
            onClicked: dialog.close()
        }
    }
    contentItem: ColumnLayout {
        spacing: 12
        ComboBox {
            id: sectionPicker
            Layout.fillWidth: true
            model: dialog.sections; textRole: "label"; valueRole: "value"
            Accessible.name: qsTranslate("Vetust", "Section")
            onActivated: dialog.error=""
        }
        Item {
            Layout.fillWidth: true; Layout.fillHeight: true
            Image {
                objectName: "selectedBackgroundPreview"
                anchors.fill: parent
                source: dialog.visible ? dialog.previewSource : ""
                fillMode: Image.PreserveAspectFit
                asynchronous: true
            }
        }
        Label { visible: text.length > 0; text: dialog.error; color: "#ffb1a4"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        RowLayout {
            Button { text: qsTranslate("Vetust", "Choose image…"); onClicked: picker.open() }
            Button {
                text: qsTranslate("Vetust", "Restore default background")
                enabled: !!vetust.sectionBackgrounds[dialog.section]
                onClicked: dialog.error=vetust.setSectionBackground(dialog.section,"")
            }
        }
    }
    Dialogs.FileDialog {
        id: picker
        title: qsTranslate("Vetust", "Choose image…")
        nameFilters: [qsTranslate("Vetust", "Images") + " (*.png *.jpg *.jpeg *.webp *.bmp *.PNG *.JPG *.JPEG *.WEBP *.BMP)"]
        onAccepted: dialog.error=vetust.setSectionBackground(dialog.section,selectedFile)
    }
}
