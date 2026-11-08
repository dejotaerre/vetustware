import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs

Dialog {
    id: editor
    objectName: "gameCustomizationDialog"
    property string gameId: ""
    property var fields: ({})
    property var descriptions: ({})
    property bool customized: false
    property string error: ""
    property string imageTarget: ""
    property int imageIndex: -1
    readonly property var languageCodes: ["es", "en", "pt_BR"]
    parent: Overlay.overlay
    x: 0; y: 0; width: parent.width; height: parent.height; margins: 0
    modal: true; focus: true; closePolicy: Popup.CloseOnEscape
    palette.window: "#111e29"; palette.windowText: "#e7edf1"; palette.text: "#e7edf1"
    palette.button: "#233443"; palette.buttonText: "#e7edf1"; palette.base: "#0b151e"
    background: Rectangle { color: "#111e29" }

    function begin(id) {
        gameId=id
        const data=vetust.gameCustomization(id)
        error=data.error || ""
        fields=data.fields || {}
        descriptions=data.descriptions || {}
        customized=!!data.customized
        languagePicker.currentIndex=Math.max(0,languageCodes.indexOf(languages.current))
        descriptionInput.text=descriptions[languageCodes[languagePicker.currentIndex]] || ""
        tabs.currentIndex=0
        open()
    }
    function setField(key,value) {
        const copy=Object.assign({},fields); copy[key]=value; fields=copy
    }
    function preview(ref) { return !ref ? "" : ref.startsWith("file:") ? ref : "image://art/"+ref }
    function choose(key,index) { imageTarget=key; imageIndex=index; picker.open() }
    function alterShot(index,delta) {
        const shots=(fields.shots || []).slice()
        if(delta===0) shots.splice(index,1)
        else { const other=index+delta; if(other<0 || other>=shots.length)return; const item=shots[index]; shots[index]=shots[other]; shots[other]=item }
        setField("shots",shots)
    }
    header: RowLayout {
        Label { Layout.fillWidth: true; Layout.margins: 16; text: qsTranslate("Vetust", "Customize details")+" · "+(editor.fields.title || ""); textFormat: Text.PlainText; elide: Text.ElideRight; font.bold: true; font.pixelSize: 18 }
        ToolButton { objectName: "closeCustomization"; text: "×"; font.pixelSize: 28; Layout.rightMargin: 12; Accessible.name: qsTranslate("Vetust", "Close"); onClicked: editor.close() }
    }
    contentItem: ColumnLayout {
        spacing: 12
        Label { Layout.fillWidth: true; text: qsTranslate("Vetust", "Changes are saved with this DOSBox library. Game settings and saves are preserved."); wrapMode: Text.WordWrap; color: "#a9bdcb" }
        TabBar {
            id: tabs; Layout.fillWidth: true
            TabButton { text: qsTranslate("Vetust", "Information") }
            TabButton { objectName: "customizationImagesTab"; text: qsTranslate("Vetust", "Images") }
        }
        StackLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; currentIndex: tabs.currentIndex
            ScrollView {
                id: informationScroll; clip: true; contentWidth: availableWidth
                ColumnLayout {
                    width: informationScroll.availableWidth; spacing: 12
                    GridLayout {
                        Layout.fillWidth: true; columns: editor.width<850 ? 1 : 2; columnSpacing: 20
                        Repeater {
                            model: [{key:"title",label:qsTranslate("Vetust", "Title")},{key:"year",label:qsTranslate("Vetust", "Year")},{key:"developer",label:qsTranslate("Vetust", "Developer")},{key:"publisher",label:qsTranslate("Vetust", "Publisher")},{key:"genre",label:qsTranslate("Vetust", "Genres (separate with semicolons)")}]
                            delegate: ColumnLayout {
                                required property var modelData
                                Layout.fillWidth: true
                                Label { text: modelData.label }
                                TextField { objectName: "customField_"+modelData.key; Layout.fillWidth: true; text: editor.fields[modelData.key] || ""; selectByMouse: true; maximumLength: 1000; onTextEdited: editor.setField(modelData.key,text) }
                            }
                        }
                    }
                    RowLayout {
                        Label { text: qsTranslate("Vetust", "Description") }
                        ComboBox { id: languagePicker; objectName: "customDescriptionLanguage"; model: ["Español","English","Português (Brasil)"]; onActivated: descriptionInput.text=editor.descriptions[editor.languageCodes[currentIndex]] || "" }
                    }
                    Label { Layout.fillWidth: true; text: qsTranslate("Vetust", "Each language has its own description. Editing does not translate the text automatically."); wrapMode: Text.WordWrap; color: "#a9bdcb" }
                    TextArea {
                        id: descriptionInput; objectName: "customDescription"
                        Layout.fillWidth: true; Layout.preferredHeight: Math.max(230,editor.height-460)
                        wrapMode: TextEdit.Wrap; textFormat: TextEdit.PlainText; selectByMouse: true
                        background: Rectangle { color: "#0b151e"; border.color: "#3e5666"; radius: 4 }
                        onTextChanged: { const copy=Object.assign({},editor.descriptions); copy[editor.languageCodes[languagePicker.currentIndex]]=text; editor.descriptions=copy }
                    }
                }
            }
            ScrollView {
                id: imagesScroll; clip: true; contentWidth: availableWidth
                ColumnLayout {
                    width: imagesScroll.availableWidth; spacing: 16
                    Label { Layout.fillWidth: true; text: qsTranslate("Vetust", "Choose normal images. The details background is blurred automatically; without a background, the cover is used."); wrapMode: Text.WordWrap; color: "#a9bdcb" }
                    GridLayout {
                        Layout.fillWidth: true; columns: editor.width<750 ? 1 : 3; columnSpacing: 16
                        Repeater {
                            model: [{key:"cover",label:qsTranslate("Vetust", "Cover")},{key:"box3d",label:qsTranslate("Vetust", "3D box")},{key:"background",label:qsTranslate("Vetust", "Background")}]
                            delegate: ColumnLayout {
                                required property var modelData
                                Layout.fillWidth: true
                                Label { text: modelData.label; font.bold: true }
                                Rectangle {
                                    Layout.fillWidth: true; Layout.preferredHeight: 240; color: "#0b151e"; radius: 6
                                    Image { anchors.fill: parent; anchors.margins: 8; source: editor.preview(editor.fields[modelData.key]); fillMode: Image.PreserveAspectFit; asynchronous: true }
                                    Label { anchors.centerIn: parent; visible: !editor.fields[modelData.key]; text: qsTranslate("Vetust", "No image"); color: "#8298aa" }
                                }
                                RowLayout {
                                    Button { text: qsTranslate("Vetust", "Choose image…"); onClicked: editor.choose(modelData.key,-1) }
                                    Button { text: qsTranslate("Vetust", "Remove"); enabled: !!editor.fields[modelData.key]; onClicked: editor.setField(modelData.key,"") }
                                }
                            }
                        }
                    }
                    RowLayout {
                        Label { text: qsTranslate("Vetust", "Gallery"); font.bold: true; Layout.fillWidth: true }
                        Button { objectName: "addCustomizationShot"; text: qsTranslate("Vetust", "Add image…"); onClicked: editor.choose("shots",-1) }
                    }
                    GridLayout {
                        Layout.fillWidth: true; columns: Math.max(1,Math.floor(imagesScroll.availableWidth/290)); columnSpacing: 16; rowSpacing: 16
                        Repeater {
                            model: editor.fields.shots || []
                            delegate: ColumnLayout {
                                required property string modelData
                                required property int index
                                Layout.fillWidth: true
                                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 170; color: "#0b151e"; radius: 6
                                    Image { anchors.fill: parent; anchors.margins: 8; source: editor.preview(modelData); fillMode: Image.PreserveAspectFit; asynchronous: true }
                                }
                                RowLayout {
                                    Button { text: "←"; implicitWidth: 40; enabled: index>0; Accessible.name: qsTranslate("Vetust", "Move earlier"); onClicked: editor.alterShot(index,-1) }
                                    Button { text: "→"; implicitWidth: 40; enabled: index<(editor.fields.shots || []).length-1; Accessible.name: qsTranslate("Vetust", "Move later"); onClicked: editor.alterShot(index,1) }
                                    Button { text: qsTranslate("Vetust", "Replace"); onClicked: editor.choose("shots",index) }
                                    Button { text: "×"; implicitWidth: 40; Accessible.name: qsTranslate("Vetust", "Remove"); onClicked: editor.alterShot(index,0) }
                                }
                            }
                        }
                    }
                }
            }
        }
        Label { Layout.fillWidth: true; text: editor.error; textFormat: Text.PlainText; color: "#ffb1a4"; visible: text.length>0; wrapMode: Text.WordWrap }
    }
    footer: RowLayout {
        Button { Layout.margins: 12; text: qsTranslate("Vetust", "Restore original details"); enabled: editor.customized; onClicked: restoreDialog.open() }
        Item { Layout.fillWidth: true }
        Button { text: qsTranslate("Vetust", "Cancel"); onClicked: editor.close() }
        Button { objectName: "saveCustomization"; Layout.margins: 12; text: qsTranslate("Vetust", "Save"); enabled: !vetust.busy; highlighted: true; onClicked: { editor.error=vetust.saveGameCustomization(editor.gameId,{fields:editor.fields,descriptions:editor.descriptions}); if(!editor.error)editor.close() } }
    }
    Dialogs.FileDialog {
        id: picker; title: qsTranslate("Vetust", "Choose image…")
        nameFilters: [qsTranslate("Vetust", "Images")+" (*.png *.jpg *.jpeg *.webp *.bmp *.PNG *.JPG *.JPEG *.WEBP *.BMP)"]
        onAccepted: {
            if(editor.imageTarget==="shots") {
                const shots=(editor.fields.shots || []).slice()
                if(editor.imageIndex<0)shots.push(selectedFile.toString());else shots[editor.imageIndex]=selectedFile.toString()
                editor.setField("shots",shots)
            } else editor.setField(editor.imageTarget,selectedFile.toString())
        }
    }
    Dialog {
        id: restoreDialog; parent: Overlay.overlay; anchors.centerIn: parent; width: Math.min(parent.width-40,530); height: 230
        modal: true; focus: true; title: qsTranslate("Vetust", "Restore original details")
        standardButtons: Dialog.Yes | Dialog.No
        contentItem: Label { text: qsTranslate("Vetust", "Discard saved customizations for this game and restore the library's original details and images?"); wrapMode: Text.WordWrap }
        onAccepted: { editor.error=vetust.resetGameCustomization(editor.gameId); if(!editor.error)editor.begin(editor.gameId) }
    }
}
