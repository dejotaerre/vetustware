import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: dialog
    objectName: "storageDialog"
    parent: Overlay.overlay
    x: 0; y: 0; width: parent.width; height: parent.height; margins: 0
    focus: true
    header: Item {
        implicitHeight: 56
        RowLayout {
            anchors.fill: parent; anchors.margins: 12
            Label { Layout.fillWidth: true; text: dialog.title; color: "#e7edf1"; font.bold: true; font.pixelSize: 16; elide: Text.ElideRight }
            ToolButton {
                objectName: "closeStorageButton"
                text: "×"; font.pixelSize: 26; implicitWidth: 36; implicitHeight: 32
                Accessible.name: qsTranslate("Vetust", "Close")
                ToolTip.text: qsTranslate("Vetust", "Close (Esc)"); ToolTip.visible: hovered
                onClicked: dialog.close()
            }
        }
    }
    modal: true; closePolicy: Popup.CloseOnEscape
    title: qsTranslate("Vetust", "Libraries and storage")
    palette.window: "#111e29"; palette.windowText: "#e7edf1"
    palette.text: "#e7edf1"; palette.base: "#0b151f"
    palette.button: "#233443"; palette.buttonText: "#e7edf1"
    property string error: ""
    property var detected: []
    property bool loading: false
    property string legacyBase: ""
    function locations() { return {dosbox:dos.path, scummvm:scumm.path, atari2600:atari.path, mame:arcade.path, magazines:magazines.path,embeddedMetadata:true,embeddedResources:true} }
    function inspect() { if(!loading) detected=vetust.inspectLibraries(legacyBase,locations()) }
    function status(key) { return detected.find(row=>row.key===key) || ({path:"",available:false,automatic:true}) }
    function begin() {
        loading=true
        const settings=vetust.storageSettings
        legacyBase=settings.legacyLayout ? settings.library : ""
        installed.text=settings.games || ""
        const chosen={dosbox:settings.dosbox || "",scummvm:settings.scummvm || "",atari2600:vetust.atari.settings.folder || "",mame:vetust.arcade.settings.folder || "",magazines:settings.magazines || ""}
        const resolved=vetust.inspectLibraries(settings.library || "",chosen)
        for(const row of resolved) {
            const target=({dosbox:dos,scummvm:scumm,atari2600:atari,mame:arcade,magazines:magazines})[row.key]
            target.path=legacyBase ? (chosen[row.key] || "") : row.path
        }
        advanced.checked=false; error=""; loading=false
        inspect(); open(); scroll.contentItem.contentY=0
    }
    function localUrl(path) { return path ? "file://"+encodeURI(path).replace(/#/g,"%23").replace(/\?/g,"%3F") : "" }
    function choose(kind,path) {
        chooser.kind=kind
        if(path)chooser.currentFolder=localUrl(path)
        chooser.open()
    }
    Timer { id: refresh; interval: 200; onTriggered: dialog.inspect() }
    FolderDialog {
        id: chooser
        property string kind: "library"
        title: qsTranslate("Vetust", "Choose folder…")
        onAccepted: {
            const path=decodeURIComponent(selectedFolder.toString().replace(/^file:\/\//,""))
            if(kind==="detect"){
                const rows=vetust.inspectLibraries(path,{})
                if(rows.some(row=>row.available)){
                    legacyBase=""
                    for(const row of rows) if(row.available)
                        ({dosbox:dos,scummvm:scumm,atari2600:atari,mame:arcade,magazines:magazines})[row.key].path=row.path
                }else dialog.error=qsTranslate("Vetust", "Choose a folder containing your collections.")
            }
            else if(kind==="games")installed.text=path
            else ({dosbox:dos,scummvm:scumm,atari2600:atari,mame:arcade,magazines:magazines})[kind].path=path
            dialog.inspect()
        }
    }
    component LibraryRow: Rectangle {
        id: row
        required property string key
        required property string label
        required property string symbol
        property alias path: custom.text
        readonly property var state: dialog.status(key)
        Layout.fillWidth: true; Layout.preferredWidth: 1
        implicitHeight: body.implicitHeight+12
        radius: 8; color: "#182734"; border.color: "#2d4251"
        RowLayout {
            id: body
            anchors.fill: parent; anchors.margins: 6; spacing: 14
            Rectangle {
                Layout.preferredWidth: 44; Layout.preferredHeight: 40
                radius: 8; color: "#233c49"
                Label { anchors.centerIn: parent; text: row.symbol; color: "#78d8d1"; font.pixelSize: 17; font.bold: true }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 5
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: row.label; font.bold: true; color: "#e7edf1" }
                    Item { Layout.fillWidth: true }
                    Label { text: row.state.available ? qsTranslate("Vetust", "Found") : qsTranslate("Vetust", "Not found"); color: row.state.available ? "#78d8d1" : "#e9bd79"; font.pixelSize: 11 }
                }
                Label {
                    Layout.fillWidth: true; visible: !advanced.checked
                    text: row.state.path || qsTranslate("Vetust", "No folder selected")
                    textFormat: Text.PlainText; elide: Text.ElideMiddle; color: "#a8bac8"; font.pixelSize: 12
                    ToolTip.text: text; ToolTip.visible: hover.hovered
                    HoverHandler { id: hover }
                }
                TextField {
                    id: custom
                    objectName: row.key==="scummvm" ? "scummFolder" : row.key+"Folder"
                    Layout.fillWidth: true; visible: advanced.checked; selectByMouse: true
                    placeholderText: row.state.path || qsTranslate("Vetust", "No folder selected")
                    onTextChanged: refresh.restart()
                }
            }
            Button { objectName: "choose"+row.key; text: qsTranslate("Vetust", "Change…"); enabled: !vetust.busy; onClicked: dialog.choose(row.key,row.state.path) }
            ToolButton {
                objectName: row.key==="scummvm" ? "resetScummFolder" : "reset"+row.key
                text: "×"; enabled: row.path.length>0; implicitWidth: 36
                Accessible.name: qsTranslate("Vetust", "Clear location")
                ToolTip.text: qsTranslate("Vetust", "Clear location"); ToolTip.visible: hovered
                onClicked: {row.path="";dialog.inspect()}
            }
        }
    }
    contentItem: ScrollView {
        id: scroll
        clip: true; contentWidth: availableWidth
        ColumnLayout {
            width: scroll.availableWidth; spacing: 12
            RowLayout {
                Layout.fillWidth: true
                Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTranslate("Vetust", "Each collection has its own location. They can be on different drives."); color: "#a8bac8" }
                Button { text: qsTranslate("Vetust", "Detect folders…"); enabled: !vetust.busy; onClicked: dialog.choose("detect",vetust.storageSettings.library || "") }
            }
            GridLayout {
                Layout.fillWidth: true; columns: dialog.width>=1100 ? 2 : 1; columnSpacing: 16; rowSpacing: 12
                LibraryRow { id: dos; key: "dosbox"; label: "DOSBox"; symbol: ">_" }
                LibraryRow { id: scumm; key: "scummvm"; label: "ScummVM"; symbol: "↖" }
                LibraryRow { id: atari; key: "atari2600"; label: "Atari 2600"; symbol: "2600" }
                LibraryRow { id: arcade; key: "mame"; label: "Arcade · MAME"; symbol: "M" }
                LibraryRow { id: magazines; key: "magazines"; label: qsTranslate("Vetust", "Magazines"); symbol: "▤"; Layout.columnSpan: parent.columns }
            }
            Label { Layout.fillWidth: true; text: qsTranslate("Vetust", "Changing a location never moves files. Missing collections can be connected later."); wrapMode: Text.WordWrap; color: "#91a3b3"; font.pixelSize: 12 }
            Label { Layout.fillWidth: true; text: qsTranslate("Vetust", "Covers, manuals and catalogs are in metadata. Emulator resources, when needed, are in resources. Both folders are detected inside each collection automatically."); wrapMode: Text.WordWrap; color: "#91a3b3"; font.pixelSize: 12 }
            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#2d4251" }
            Label { text: qsTranslate("Vetust", "Saves and personal data"); color: "#e9bd79"; font.bold: true }
            Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTranslate("Vetust", "Settings and saves stay with VETUSTware, separate from the shared library."); color: "#a8bac8" }
            CheckBox { id: advanced; objectName: "storageAdvanced"; text: qsTranslate("Vetust", "Advanced"); }
            Label { Layout.fillWidth: true; visible: advanced.checked; text: qsTranslate("Vetust", "Application data: ")+(vetust.storageSettings.personal || ""); color: "#91a3b3"; wrapMode: Text.WrapAnywhere }
            Label { visible: advanced.checked; text: qsTranslate("Vetust", "DOSBox playable copies and saves"); color: "#e7edf1" }
            RowLayout {
                visible: advanced.checked
                TextField { id: installed; objectName: "installedGamesFolder"; Layout.fillWidth: true; selectByMouse: true }
                Button { text: qsTranslate("Vetust", "Change…"); enabled: !vetust.busy; onClicked: dialog.choose("games",installed.text) }
            }
            Label { Layout.fillWidth: true; visible: advanced.checked; text: qsTranslate("Vetust", "Copy your installed DOSBox games and saves before changing their location. Other engines keep their personal data with the application."); color: "#e9bd79"; wrapMode: Text.WordWrap }
        }
    }
    footer: ColumnLayout {
        spacing: 6
        Label { visible: dialog.error!==""; Layout.fillWidth: true; Layout.margins: 12; wrapMode: Text.WordWrap; text: dialog.error; color: "#ffb1a4" }
        RowLayout {
            Layout.fillWidth: true; Layout.margins: 12
            Item { Layout.fillWidth: true }
            Button { text: qsTranslate("Vetust", "Cancel"); onClicked: dialog.close() }
            Button {
                objectName: "saveStorageButton"; text: qsTranslate("Vetust", "Save"); enabled: !vetust.busy
                onClicked: {
                    const paths=dialog.locations()
                    delete paths.scummvm
                    if(legacyBase){delete paths.embeddedMetadata;delete paths.embeddedResources}
                    dialog.error=vetust.saveStorage(dialog.localUrl(legacyBase),dialog.localUrl(installed.text),dialog.localUrl(scumm.path),paths)
                    if(dialog.error==="")dialog.close()
                }
            }
        }
    }
}
