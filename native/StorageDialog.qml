import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: dialog
    objectName:"storageDialog"
    anchors.centerIn:parent
    width:Math.min(850,parent.width-50)
    modal:true
    title:qsTranslate("Vetust", "Library and folders")
    palette.window:"#111e29"; palette.windowText:"#e7edf1"
    palette.text:"#e7edf1"; palette.base:"#0b151f"
    palette.button:"#233443"; palette.buttonText:"#e7edf1"
    property string error:""
    function begin() {
        original.text=vetust.storageSettings.library || ""
        installed.text=vetust.storageSettings.games || ""
        error=""
        open()
    }
    function localUrl(path) {return "file://"+encodeURI(path).replace(/#/g,"%23").replace(/\?/g,"%3F")}
    FolderDialog {
        id: chooser
        property bool chooseOriginal:true
        title:chooseOriginal ? qsTranslate("Vetust", "Select the VETUSTware library") : qsTranslate("Vetust", "Select the installed games folder")
        onAccepted: {
            let path=decodeURIComponent(selectedFolder.toString().replace(/^file:\/\//,""))
            if(chooseOriginal)original.text=path
            else installed.text=path
        }
    }
    contentItem:ColumnLayout {
        spacing:12
        Label { Layout.fillWidth:true; wrapMode:Text.WordWrap; text:qsTranslate("Vetust", "Choose your library. Games are prepared when you press Play; your settings and saves are stored with the application, outside the shared library."); color:"#a8bac8" }
        Label { text:qsTranslate("Vetust", "Game library") }
        RowLayout {
            TextField { id:original; Layout.fillWidth:true; placeholderText:qsTranslate("Vetust", "Folder containing Content and eXo"); selectByMouse:true }
            Button { text:qsTranslate("Vetust", "Choose…"); onClicked:{chooser.chooseOriginal=true;chooser.open()} }
        }
        CheckBox { id:advanced; text:qsTranslate("Vetust", "Choose another folder for playable copies and saves") }
        Label { visible:advanced.checked; text:qsTranslate("Vetust", "Installed games and saves") }
        RowLayout {
            visible:advanced.checked
            TextField { id:installed; Layout.fillWidth:true; placeholderText:qsTranslate("Vetust", "Destination folder for playing"); selectByMouse:true }
            Button { text:qsTranslate("Vetust", "Choose…"); onClicked:{chooser.chooseOriginal=false;chooser.open()} }
        }
        Label { Layout.fillWidth:true; wrapMode:Text.WordWrap; text:advanced.checked ? qsTranslate("Vetust", "Changing this location does not move any files. Copy your games and saves first if you have already used them.") : qsTranslate("Vetust", "Playable copies and saves: ")+installed.text; color:"#a8bac8" }
        Label { Layout.fillWidth:true; wrapMode:Text.WordWrap; text:vetust.storageSettings.status || ""; color:"#78d8d1" }
        Label { visible:dialog.error!==""; Layout.fillWidth:true; wrapMode:Text.WordWrap; text:dialog.error; color:"#ffb1a4" }
        RowLayout {
            Item { Layout.fillWidth:true }
            Button { text:qsTranslate("Vetust", "Cancel"); onClicked:dialog.close() }
            Button { text:qsTranslate("Vetust", "Save"); enabled:!vetust.busy; onClicked:{dialog.error=vetust.saveStorage(dialog.localUrl(original.text),dialog.localUrl(installed.text));if(dialog.error==="")dialog.close()} }
        }
    }
}
