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
    title:"Biblioteca y carpetas"
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
        title:chooseOriginal ? "Selecciona la biblioteca de VETUSTware" : "Selecciona la carpeta de juegos instalados"
        onAccepted: {
            let path=decodeURIComponent(selectedFolder.toString().replace(/^file:\/\//,""))
            if(chooseOriginal)original.text=path
            else installed.text=path
        }
    }
    contentItem:ColumnLayout {
        spacing:12
        Label { Layout.fillWidth:true; wrapMode:Text.WordWrap; text:"Elige tu biblioteca. Los juegos se preparan al pulsar Jugar; tus ajustes y partidas se guardan con el motor, fuera de la biblioteca compartida."; color:"#a8bac8" }
        Label { text:"Biblioteca de juegos" }
        RowLayout {
            TextField { id:original; Layout.fillWidth:true; placeholderText:"Carpeta con Content y eXo"; selectByMouse:true }
            Button { text:"Elegir…"; onClicked:{chooser.chooseOriginal=true;chooser.open()} }
        }
        CheckBox { id:advanced; text:"Elegir otra carpeta para las copias jugables y partidas" }
        Label { visible:advanced.checked; text:"Juegos instalados y partidas" }
        RowLayout {
            visible:advanced.checked
            TextField { id:installed; Layout.fillWidth:true; placeholderText:"Carpeta de destino para jugar"; selectByMouse:true }
            Button { text:"Elegir…"; onClicked:{chooser.chooseOriginal=false;chooser.open()} }
        }
        Label { Layout.fillWidth:true; wrapMode:Text.WordWrap; text:advanced.checked ? "Cambiar esta ubicación no mueve los archivos. Copia antes tus juegos y partidas si ya los utilizaste." : "Copias jugables y partidas: "+installed.text; color:"#a8bac8" }
        Label { Layout.fillWidth:true; wrapMode:Text.WordWrap; text:vetust.storageSettings.status || ""; color:"#78d8d1" }
        Label { visible:dialog.error!==""; Layout.fillWidth:true; wrapMode:Text.WordWrap; text:dialog.error; color:"#ffb1a4" }
        RowLayout {
            Item { Layout.fillWidth:true }
            Button { text:"Cancelar"; onClicked:dialog.close() }
            Button { text:"Guardar"; enabled:!vetust.busy; onClicked:{dialog.error=vetust.saveStorage(dialog.localUrl(original.text),dialog.localUrl(installed.text));if(dialog.error==="")dialog.close()} }
        }
    }
}
