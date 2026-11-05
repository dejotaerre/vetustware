import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ColumnLayout {
    id: page
    objectName: "atariView"
    property string query: ""
    property bool showHidden: false
    property bool favoritesOnly: false
    property bool detailsOpen: details.visible || options.visible || folderPicker.visible
    property var selected: ({})
    property var edition: selected.selected || ({})
    property bool showSnapshot: false
    property var entries: vetust.atari.games
    spacing: 12
    function showGame(entry) { selected=entry; showSnapshot=false; details.open() }
    Connections {
        target: vetust.atari
        function onChanged() {
            if(page.selected.id) {
                const updated=page.entries.find(g=>g.id===page.selected.id)
                if(updated)page.selected=updated
            }
        }
    }
    RowLayout {
        Layout.fillWidth: true; spacing: 12
        ColumnLayout {
            id: heading
            Layout.fillWidth: true; Layout.maximumWidth: Infinity; spacing: 5
            Label { text: "ATARI 2600 · STELLA"; color: "#78d8d1"; font.pixelSize: 11; font.letterSpacing: 2 }
            Label { text: qsTranslate("Vetust", "Small cartridges, great memories"); color: "#e9bd79"; font.pixelSize: 26; font.family: "Noto Serif" }
            Label { text: qsTranslate("Vetust", "%1 titles · %2 unique ROMs").arg(page.entries.length).arg(vetust.atari.settings.uniqueRoms); color: "#91a3b3" }
        }
        Image {
            objectName: "atariHeaderArt"
            property real availableWidth: page.width-heading.implicitWidth-headerActions.implicitWidth-48
            visible: availableWidth >= 160
            Layout.preferredWidth: Math.min(280, Math.max(0, availableWidth))
            Layout.preferredHeight: Layout.preferredWidth*9/16
            Layout.alignment: Qt.AlignVCenter
            source: "assets/atari2600-header.png"
            sourceSize.width: 720
            fillMode: Image.PreserveAspectFit
            Accessible.ignored: true
        }
        RowLayout {
            id: headerActions
            spacing: 12
            Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; objectName: "scanAtari"; text: qsTranslate("Vetust", "Update catalog"); enabled: !vetust.busy; onClicked: vetust.atari.scan() }
            Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; objectName: "atariSettings"; text: qsTranslate("Vetust", "Atari settings"); enabled: !vetust.busy; onClicked: options.open() }
            ToolButton { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; text: "↗"; Accessible.name: qsTranslate("Vetust", "Open library folder"); ToolTip.text: Accessible.name; ToolTip.visible: hovered; onClicked: vetust.atari.openFolder() }
        }
    }
    RowLayout {
        Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; objectName: "showHiddenAtari"; text: qsTranslate("Vetust", "Show hidden"); checkable: true; checked: page.showHidden; onClicked: page.showHidden=checked }
        Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; objectName: "favoritesAtari"; text: "♥  "+qsTranslate("Vetust", "FAVORITES"); checkable: true; checked: page.favoritesOnly; onClicked: page.favoritesOnly=checked }
        Label { Layout.fillWidth: true; text: qsTranslate("Vetust", "Variants share a card. Your original files are preserved."); color: "#91a3b3"; wrapMode: Text.WordWrap }
    }
    Label { Layout.fillWidth: true; visible: text.length>0; text: vetust.atari.status; color: "#78d8d1"; wrapMode: Text.WordWrap; textFormat: Text.PlainText }
    GridView {
        id: grid; objectName: "atariGrid"
        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
        cellWidth: Math.floor(width/Math.max(3,Math.floor(width/220))); cellHeight: 380
        model: page.entries.filter(g=>(page.showHidden || !g.hidden) && (!page.favoritesOnly || g.favorite) && (g.title+" "+g.variants.map(v=>v.title+" "+v.manufacturer).join(" ")).toLocaleLowerCase().includes(page.query.toLocaleLowerCase()))
        boundsBehavior: Flickable.StopAtBounds; cacheBuffer: 300
        ScrollBar.vertical: ScrollBar {}
        delegate: Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1";
            id: tile
            required property var modelData
            required property int index
            width: grid.cellWidth-16; height: grid.cellHeight-16; padding: 10; hoverEnabled: true
            objectName: "atariTile-"+index
            Accessible.name: modelData.title
            onClicked: page.showGame(modelData)
            onActiveFocusChanged: if(activeFocus)grid.currentIndex=index
            Keys.onRightPressed: {grid.moveCurrentIndexRight();if(grid.currentItem)grid.currentItem.forceActiveFocus()}
            Keys.onLeftPressed: {grid.moveCurrentIndexLeft();if(grid.currentItem)grid.currentItem.forceActiveFocus()}
            Keys.onDownPressed: {grid.moveCurrentIndexDown();if(grid.currentItem)grid.currentItem.forceActiveFocus()}
            Keys.onUpPressed: {grid.moveCurrentIndexUp();if(grid.currentItem)grid.currentItem.forceActiveFocus()}
            Keys.onReturnPressed: clicked()
            background: Rectangle { radius: 7; color: tile.hovered ? "#1b2b38" : "#101c27"; border.color: tile.activeFocus ? "#78d8d1" : tile.hovered ? "#e9bd79" : "#2d3e4b" }
            contentItem: ColumnLayout {
                spacing: 8
                Rectangle {
                    Layout.fillWidth: true; Layout.fillHeight: true; color: "#080f17"; radius: 4
                    Image { anchors.fill: parent; anchors.margins: 5; source: tile.modelData.cover || ""; fillMode: Image.PreserveAspectFit; asynchronous: true; sourceSize.width: 400 }
                    Label { anchors.centerIn: parent; width: parent.width-18; visible: !tile.modelData.cover; text: tile.modelData.title; textFormat: Text.PlainText; color: "#e9bd79"; font.pixelSize: 20; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap }
                    Label { anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 6; text: (tile.modelData.favorite?"♥  ":"")+(tile.modelData.hidden?qsTranslate("Vetust", "Hidden"):""); color: "#e9bd79"; style: Text.Outline; styleColor: "#080f17" }
                }
                Label { Layout.fillWidth: true; text: tile.modelData.title; textFormat: Text.PlainText; color: "#e7edf1"; elide: Text.ElideRight; font.pixelSize: 13 }
                Label { Layout.fillWidth: true; text: (tile.modelData.selected.year || "")+" · "+(tile.modelData.selected.manufacturer || "Atari 2600"); textFormat: Text.PlainText; color: "#91a3b3"; elide: Text.ElideRight; font.pixelSize: 10 }
                Label { text: tile.modelData.variants.length===1 ? qsTranslate("Vetust", "1 variant") : qsTranslate("Vetust", "%1 variants").arg(tile.modelData.variants.length); color: "#78d8d1"; font.pixelSize: 11 }
            }
        }
        Label { anchors.centerIn: parent; width: Math.min(600,parent.width-40); visible: grid.count===0; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; color: "#91a3b3"; text: page.entries.length>0 ? qsTranslate("Vetust", "No games match these filters.") : qsTranslate("Vetust", "Choose your Atari library folder in Atari settings.") }
    }
    Dialog {
        id: details; objectName: "atariDetails"
        background: LibraryBackdrop { source: page.edition.cover || page.edition.image || "" }
        parent: Overlay.overlay; x: 0; y: 0; width: parent.width; height: parent.height; margins: 0
        modal: true; focus: true; footer: null
        palette.window: "#101c27"; palette.windowText: "#e7edf1"; palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; palette.base: "#1b2d3b"; palette.text: "#e7edf1"
        header: Item {
            implicitHeight: 56
            RowLayout {
                anchors.fill: parent; anchors.margins: 12
                Label { Layout.fillWidth: true; text: page.selected.title || "Atari 2600"; textFormat: Text.PlainText; color: "#e9bd79"; font.pixelSize: 17; elide: Text.ElideRight }
                ToolButton { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; objectName: "closeAtariDetails"; text: "×"; font.pixelSize: 26; implicitWidth: 36; Accessible.name: qsTranslate("Vetust", "Close"); ToolTip.text: qsTranslate("Vetust", "Close (Esc)"); ToolTip.visible: hovered; onClicked: details.close() }
            }
        }
        contentItem: RowLayout {
            spacing: 24
            ColumnLayout {
                Layout.preferredWidth: Math.min(320,details.width*0.38); Layout.fillHeight: true
                RowLayout {
                    Button { objectName: "atariBoxButton"; text: qsTranslate("Vetust", "Box art"); checkable: true; checked: !page.showSnapshot; onClicked: page.showSnapshot=false; palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1" }
                    Button { objectName: "atariSnapshotButton"; text: qsTranslate("Vetust", "Screenshot"); checkable: true; checked: page.showSnapshot; enabled: !!page.edition.image; onClicked: page.showSnapshot=true; palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1" }
                }
                Item {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    Image { objectName: "atariDetailImage"; anchors.fill: parent; source: page.showSnapshot ? (page.edition.image || "") : (page.edition.cover || ""); fillMode: Image.PreserveAspectFit; asynchronous: true; sourceSize.width: 700
                        MouseArea {
                            objectName: "atariDetailPlayImage"
                            anchors.fill: parent; enabled: playAtariButton.enabled
                            cursorShape: Qt.PointingHandCursor; hoverEnabled: true
                            ToolTip.text: playAtariButton.text; ToolTip.visible: containsMouse && enabled
                            onClicked: playAtariButton.clicked()
                        }
                    }
                    Label { anchors.centerIn: parent; width: parent.width-20; visible: !page.showSnapshot && !page.edition.cover; text: qsTranslate("Vetust", "No box art matched yet."); color: "#91a3b3"; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 14
                Label { text: (page.edition.year || "")+" · "+(page.edition.manufacturer || "Atari 2600"); Layout.fillWidth: true; wrapMode: Text.WordWrap; textFormat: Text.PlainText; color: "#78d8d1" }
                Label { text: qsTranslate("Vetust", "Edition"); color: "#91a3b3" }
                ComboBox {
                    objectName: "atariEdition"; Layout.fillWidth: true; model: page.selected.variants || []; textRole: "title"; valueRole: "id"; enabled: !vetust.busy
                    currentIndex: (page.selected.variants || []).findIndex(v=>v.id===page.edition.id)
                    onActivated: vetust.atari.choose(page.selected.id,currentValue)
                }
                Label { Layout.fillWidth: true; text: [page.edition.controller,page.edition.format].filter(v=>v && v!=="AUTO").join(" · "); visible: text.length>0; color: "#91a3b3"; textFormat: Text.PlainText }
                Label { Layout.fillWidth: true; text: page.edition.note || ""; visible: text.length>0; wrapMode: Text.WordWrap; textFormat: Text.PlainText; color: "#91a3b3" }
                RowLayout {
                    Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; objectName: "favoriteAtariGame"; text: page.selected.favorite ? "♥" : "♡"; Accessible.name: qsTranslate("Vetust", "FAVORITES"); onClicked: vetust.atari.setFavorite(page.selected.id,!page.selected.favorite) }
                    Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; objectName: "hideAtariGame"; text: page.selected.hidden ? qsTranslate("Vetust", "Unhide game") : qsTranslate("Vetust", "Hide game"); onClicked: {vetust.atari.setHidden(page.selected.id,!page.selected.hidden);details.close()} }
                }
                Item { Layout.fillHeight: true }
                Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; id: playAtariButton; objectName: "playAtari"; text: qsTranslate("Vetust", "Play in Stella →"); enabled: !!page.edition.id && !vetust.busy && vetust.atari.available; onClicked: {vetust.atari.play(page.selected.id,page.edition.id);details.close()} }
                Label { Layout.fillWidth: true; text: qsTranslate("Vetust", "Tab: controller settings · F2: reset/start · F9: save state · F11: load state."); color: "#91a3b3"; wrapMode: Text.WordWrap }
            }
        }
    }
    Dialog {
        id: options; objectName: "atariOptions"
        property url folder: ""
        anchors.centerIn: Overlay.overlay; width: Math.min(750,Overlay.overlay.width-60)
        title: qsTranslate("Vetust", "Atari settings"); modal: true; focus: true; standardButtons: Dialog.NoButton
        palette.window: "#101c27"; palette.windowText: "#e7edf1"; palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; palette.base: "#1b2d3b"; palette.text: "#e7edf1"
        onOpened: { folder=vetust.atari.settings.folderUrl; fullscreen.checked=vetust.atari.settings.fullscreen; picture.currentIndex=vetust.atari.settings.crt?1:0; error.text="" }
        contentItem: ColumnLayout {
            spacing: 14
            Label { text: qsTranslate("Vetust", "Atari library folder"); color: "#e7edf1" }
            Label { Layout.fillWidth: true; text: options.folder.toString() || vetust.atari.settings.resolvedFolder; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; color: "#91a3b3" }
            RowLayout {
                Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; text: qsTranslate("Vetust", "Choose folder…"); onClicked: folderPicker.open() }
                Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; text: qsTranslate("Vetust", "Use the main library"); onClicked: options.folder="" }
            }
            Switch { id: fullscreen; text: qsTranslate("Vetust", "Fullscreen") }
            RowLayout {
                Label { text: qsTranslate("Vetust", "Picture"); color: "#e7edf1" }
                ComboBox { id: picture; model: [qsTranslate("Vetust", "Sharp"),qsTranslate("Vetust", "Classic monitor")] }
            }
            Label { Layout.fillWidth: true; text: qsTranslate("Vetust", "Uses the monitor selected in global settings. New saves stay in this installation of VETUSTware."); color: "#91a3b3"; wrapMode: Text.WordWrap }
            Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; text: qsTranslate("Vetust", "Manage in Stella"); enabled: !vetust.busy && vetust.atari.available; onClicked: {error.text=vetust.atari.configure(options.folder,fullscreen.checked,picture.currentIndex===1);if(!error.text){vetust.atari.manage();options.close()}} }
            Label { visible: vetust.atari.settings.legacyStates; Layout.fillWidth: true; text: qsTranslate("Vetust", "Old Stella 3.2.2 states are preserved separately. Compatibility with current Stella is not guaranteed."); color: "#e9bd79"; wrapMode: Text.WordWrap }
            Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; visible: vetust.atari.settings.legacyStates; text: qsTranslate("Vetust", "Open preserved states"); onClicked: vetust.atari.openFolder(true) }
            Label { id: error; Layout.fillWidth: true; visible: text.length>0; color: "#f1a292"; wrapMode: Text.WordWrap }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; text: qsTranslate("Vetust", "Cancel"); onClicked: options.close() }
                Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; text: qsTranslate("Vetust", "Save"); onClicked: {error.text=vetust.atari.configure(options.folder,fullscreen.checked,picture.currentIndex===1);if(!error.text){options.close();vetust.atari.enter()}} }
            }
        }
    }
    FolderDialog { id: folderPicker; title: qsTranslate("Vetust", "Atari library folder"); onAccepted: options.folder=selectedFolder }
}
