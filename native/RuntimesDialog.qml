import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog
    objectName: "runtimesDialog"
    anchors.centerIn: parent
    width: Math.min(800,parent.width-50)
    height: Math.min(640,parent.height-50)
    modal: true; focus: true
    title: qsTranslate("Vetust", "Emulators and updates")
    palette.window: "#111e29"; palette.windowText: "#e7edf1"
    palette.text: "#e7edf1"; palette.base: "#0b151f"
    palette.button: "#233443"; palette.buttonText: "#e7edf1"
    standardButtons: Dialog.NoButton
    function begin(){open();vetust.runtimes.refresh()}
    header: RowLayout {
        Label { Layout.fillWidth:true; Layout.margins:14; text:dialog.title; font.bold:true; color:"#e9bd79" }
        ToolButton { text:"×"; font.pixelSize:26; Accessible.name:qsTranslate("Vetust", "Close"); onClicked:dialog.close() }
    }
    contentItem: ColumnLayout {
        spacing: 14
        Label { Layout.fillWidth:true; wrapMode:Text.WordWrap; text:qsTranslate("Vetust", "Private emulators in your VETUSTware folder. Settings and saves are stored separately."); color:"#a8bac8" }
        Label { Layout.fillWidth:true; wrapMode:Text.WordWrap; text:qsTranslate("Vetust", "Updates are installed only when you choose. The previous runtime is kept so you can go back."); color:"#91a3b3" }
        ScrollView {
            Layout.fillWidth:true; Layout.fillHeight:true; contentWidth:availableWidth; clip:true
            ColumnLayout {
                width:parent.width; spacing:14
                Repeater {
                    model:vetust.runtimes.engines
                    delegate:Rectangle {
                        required property var modelData
                        Layout.fillWidth:true; implicitHeight:runtimeContent.implicitHeight+28
                        color:"#172633"; radius:6; border.color:"#304653"
                        ColumnLayout {
                            id:runtimeContent; anchors.fill:parent; anchors.margins:14; spacing:8
                            Label { text:modelData.name; font.pixelSize:21; color:"#e9bd79" }
                            Label { text:modelData.installed ? qsTranslate("Vetust", "Installed: %1").arg(modelData.version || "—") : qsTranslate("Vetust", "No private runtime installed"); color:"#e7edf1" }
                            Label { Layout.fillWidth:true; wrapMode:Text.WordWrap; color:"#91a3b3"; text:modelData.update ? qsTranslate("Vetust", "Available: %1").arg(modelData.availableVersion) : modelData.availableVersion ? qsTranslate("Vetust", "No newer compatible runtime available") : modelData.checked ? qsTranslate("Vetust", "No compatible runtime package has been published yet.") : qsTranslate("Vetust", "Check for updates to see published compatible packages.") }
                            Flow {
                                Layout.fillWidth:true; spacing:8
                                Button { objectName:"installRuntime-"+modelData.id; visible:modelData.update; text:modelData.installed?qsTranslate("Vetust", "Install update"):qsTranslate("Vetust", "Install runtime"); enabled:!vetust.busy; onClicked:vetust.runtimes.install(modelData.id) }
                                Button { objectName:"importRuntime-"+modelData.id; visible:!modelData.installed && modelData.systemAvailable; text:qsTranslate("Vetust", "Copy installed system version"); enabled:!vetust.busy; onClicked:vetust.runtimes.importSystem(modelData.id) }
                                Button { objectName:"rollbackRuntime-"+modelData.id; visible:modelData.rollback; text:qsTranslate("Vetust", "Restore previous version (%1)").arg(modelData.previousVersion); enabled:!vetust.busy; onClicked:vetust.runtimes.rollback(modelData.id) }
                            }
                        }
                    }
                }
            }
        }
        Label { Layout.fillWidth:true; wrapMode:Text.WordWrap; text:qsTranslate("Vetust", vetust.runtimes.message); visible:text.length>0; color:vetust.runtimes.failed?"#e9bd79":"#78d8d1" }
        Label { Layout.fillWidth:true; wrapMode:Text.WrapAnywhere; maximumLineCount:4; elide:Text.ElideRight; text:vetust.runtimes.detail; visible:text.length>0; color:"#91a3b3"; textFormat:Text.PlainText }
        BusyIndicator { Layout.alignment:Qt.AlignHCenter; running:vetust.runtimes.busy; visible:running; implicitHeight:32 }
        Button { objectName:"checkRuntimeUpdates"; text:qsTranslate("Vetust", "Check for updates"); enabled:!vetust.busy; onClicked:vetust.runtimes.check() }
    }
}
