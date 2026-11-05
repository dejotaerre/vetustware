import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ColumnLayout {
    id: page
    objectName: "arcadeView"
    property string query: ""
    property bool showHidden: false
    property bool favoritesOnly: false
    property bool detailsOpen: details.visible || options.visible || folderPicker.visible
    property var selected: ({})
    property var edition: selected.selected || ({})
    property string mediaKey: "cover"
    property bool showUnavailable: false
    property string category: ""
    property var entries: vetust.arcade.games
    spacing: 12
    function categoryNames() {
        let values=[]
        for(let g of entries) for(let v of g.variants) if(v.category && values.indexOf(v.category)<0)values.push(v.category)
        return values.sort()
    }
    function showGame(entry) { selected=entry; mediaKey="cover"; details.open() }
    Connections {
        target: vetust.arcade
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
            Label { text: "ARCADE · MAME 0.241"; color: "#78d8d1"; font.pixelSize: 11; font.letterSpacing: 2 }
            Label { text: qsTranslate("Vetust", "The arcade, at home"); color: "#e9bd79"; font.pixelSize: 26; font.family: "Noto Serif" }
            Label { text: qsTranslate("Vetust", "%1 families · %2 verified editions").arg(page.entries.length).arg(vetust.arcade.settings.verified); color: "#91a3b3" }
        }
        Image {
            objectName: "arcadeHeaderArt"
            property real availableWidth: page.width-heading.implicitWidth-headerActions.implicitWidth-48
            visible: availableWidth >= 160
            Layout.preferredWidth: Math.min(280, Math.max(0, availableWidth))
            Layout.preferredHeight: Layout.preferredWidth*9/16
            Layout.alignment: Qt.AlignVCenter
            source: "assets/arcade-header.png"
            sourceSize.width: 720
            fillMode: Image.PreserveAspectFit
            Accessible.ignored: true
        }
        RowLayout {
            id: headerActions
            spacing: 12
            Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; objectName: "scanArcade"; text: qsTranslate("Vetust", "Update catalog"); enabled: !vetust.busy; onClicked: vetust.arcade.scan() }
            Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; objectName: "arcadeSettings"; text: qsTranslate("Vetust", "Arcade settings"); enabled: !vetust.busy; onClicked: options.open() }
            ToolButton { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; text: "↗"; Accessible.name: qsTranslate("Vetust", "Open library folder"); ToolTip.text: Accessible.name; ToolTip.visible: hovered; onClicked: vetust.arcade.openFolder() }
        }
    }
    RowLayout {
        Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; objectName: "showHiddenArcade"; text: qsTranslate("Vetust", "Show hidden"); checkable: true; checked: page.showHidden; onClicked: page.showHidden=checked }
        Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; objectName: "favoritesArcade"; text: "♥  "+qsTranslate("Vetust", "FAVORITES"); checkable: true; checked: page.favoritesOnly; onClicked: page.favoritesOnly=checked }
        Label { Layout.fillWidth: true; text: qsTranslate("Vetust", "MAME 0.241 · Regional editions share a card."); color: "#91a3b3"; wrapMode: Text.WordWrap }
    }
    RowLayout {
        ComboBox {
            Layout.preferredWidth: Math.min(350,page.width*0.4)
            palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; palette.base: "#1b2d3b"; palette.text: "#e7edf1"
            model: [qsTranslate("Vetust", "All categories")].concat(page.categoryNames())
            onActivated: page.category=currentIndex===0?"":currentText
        }
        CheckBox { palette.windowText: "#e7edf1"; palette.text: "#e7edf1"; palette.base: "#1b2d3b"; palette.highlight: "#78d8d1"; text: qsTranslate("Vetust", "Include incomplete or non-working editions"); checked: page.showUnavailable; onToggled: page.showUnavailable=checked }
    }
    Label { Layout.fillWidth: true; visible: text.length>0; text: vetust.arcade.status; color: "#78d8d1"; wrapMode: Text.WordWrap; textFormat: Text.PlainText }
    GridView {
        id: grid; objectName: "arcadeGrid"
        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
        cellWidth: Math.floor(width/Math.max(3,Math.floor(width/220))); cellHeight: 380
        model: page.entries.filter(g=>(page.showHidden || !g.hidden) && (page.showUnavailable || g.variants.some(v=>v.playable && v.status!=="preliminary")) && (!page.category || g.variants.some(v=>v.category===page.category)) && (!page.favoritesOnly || g.favorite) && (g.title+" "+g.variants.map(v=>v.id+" "+v.title+" "+v.manufacturer).join(" ")).toLocaleLowerCase().includes(page.query.toLocaleLowerCase()))
        boundsBehavior: Flickable.StopAtBounds; cacheBuffer: 300
        ScrollBar.vertical: ScrollBar {}
        delegate: Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1";
            id: tile
            required property var modelData
            required property int index
            width: grid.cellWidth-16; height: grid.cellHeight-16; padding: 10; hoverEnabled: true
            objectName: "arcadeTile-"+index
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
                    Image { anchors.fill: parent; anchors.margins: 5; source: tile.modelData.cover || tile.modelData.selected.cabinet || tile.modelData.image || ""; fillMode: Image.PreserveAspectFit; asynchronous: true; sourceSize.width: 400 }
                    Label { anchors.centerIn: parent; width: parent.width-18; visible: !tile.modelData.cover && !tile.modelData.selected.cabinet && !tile.modelData.image; text: tile.modelData.title; textFormat: Text.PlainText; color: "#e9bd79"; font.pixelSize: 20; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap }
                    Label { anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 6; text: (tile.modelData.favorite?"♥  ":"")+(tile.modelData.hidden?qsTranslate("Vetust", "Hidden"):""); color: "#e9bd79"; style: Text.Outline; styleColor: "#080f17" }
                }
                Label { Layout.fillWidth: true; text: tile.modelData.title; textFormat: Text.PlainText; color: "#e7edf1"; elide: Text.ElideRight; font.pixelSize: 13 }
                Label { Layout.fillWidth: true; text: (tile.modelData.selected.year || "")+" · "+(tile.modelData.selected.manufacturer || "Arcade"); textFormat: Text.PlainText; color: "#91a3b3"; elide: Text.ElideRight; font.pixelSize: 10 }
                Label { text: tile.modelData.variants.length===1 ? qsTranslate("Vetust", "1 variant") : qsTranslate("Vetust", "%1 variants").arg(tile.modelData.variants.length); color: "#78d8d1"; font.pixelSize: 11 }
            }
        }
        Label { anchors.centerIn: parent; width: Math.min(600,parent.width-40); visible: grid.count===0; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; color: "#91a3b3"; text: page.entries.length>0 ? qsTranslate("Vetust", "No games match these filters.") : qsTranslate("Vetust", "Choose your Arcade library folder in Arcade settings.") }
    }
    Dialog {
        id: details; objectName: "arcadeDetails"
        background: LibraryBackdrop { source: page.edition.cover || page.edition.titleImage || page.edition.cabinet || page.edition.image || "" }
        parent: Overlay.overlay; x: 0; y: 0; width: parent.width; height: parent.height; margins: 0
        modal: true; focus: true; footer: null
        palette.window: "#101c27"; palette.windowText: "#e7edf1"; palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; palette.base: "#1b2d3b"; palette.text: "#e7edf1"
        header: Item {
            implicitHeight: 56
            RowLayout {
                anchors.fill: parent; anchors.margins: 12
                Label { Layout.fillWidth: true; text: page.selected.title || "Arcade"; textFormat: Text.PlainText; color: "#e9bd79"; font.pixelSize: 17; elide: Text.ElideRight }
                ToolButton { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; objectName: "closeArcadeDetails"; text: "×"; font.pixelSize: 26; implicitWidth: 36; Accessible.name: qsTranslate("Vetust", "Close"); ToolTip.text: qsTranslate("Vetust", "Close (Esc)"); ToolTip.visible: hovered; onClicked: details.close() }
            }
        }
        contentItem: RowLayout {
            spacing: 24
            ColumnLayout {
                Layout.preferredWidth: Math.min(320,details.width*0.38); Layout.fillHeight: true
                ComboBox {
                    Layout.fillWidth: true
                    model: [{label:qsTranslate("Vetust", "Flyer"),key:"cover"},{label:qsTranslate("Vetust", "Screenshot"),key:"image"},{label:qsTranslate("Vetust", "Cabinet"),key:"cabinet"},{label:qsTranslate("Vetust", "Marquee"),key:"marquee"},{label:qsTranslate("Vetust", "Control panel"),key:"panel"},{label:qsTranslate("Vetust", "Title screen"),key:"titleImage"}]
                    textRole: "label"; valueRole: "key"
                    currentIndex: model.findIndex(x=>x.key===page.mediaKey)
                    onActivated: page.mediaKey=currentValue
                }
                Item {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    Image { objectName: "arcadeDetailImage"; anchors.fill: parent; source: page.edition[page.mediaKey] || ""; fillMode: Image.PreserveAspectFit; asynchronous: true; sourceSize.width: 700
                        MouseArea {
                            objectName: "arcadeDetailPlayImage"
                            anchors.fill: parent; enabled: playArcadeButton.enabled
                            cursorShape: Qt.PointingHandCursor; hoverEnabled: true
                            ToolTip.text: playArcadeButton.text; ToolTip.visible: containsMouse && enabled
                            onClicked: playArcadeButton.clicked()
                        }
                    }
                    Label { anchors.centerIn: parent; width: parent.width-20; visible: !page.edition[page.mediaKey]; text: qsTranslate("Vetust", "No image available for this view."); color: "#91a3b3"; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 14
                Label { text: (page.edition.year || "")+" · "+(page.edition.manufacturer || "Arcade"); Layout.fillWidth: true; wrapMode: Text.WordWrap; textFormat: Text.PlainText; color: "#78d8d1" }
                Label { text: qsTranslate("Vetust", "Edition"); color: "#91a3b3" }
                ComboBox {
                    objectName: "arcadeEdition"; Layout.fillWidth: true; model: page.selected.variants || []; textRole: "title"; valueRole: "id"; enabled: !vetust.busy
                    currentIndex: (page.selected.variants || []).findIndex(v=>v.id===page.edition.id)
                    onActivated: vetust.arcade.choose(page.selected.id,currentValue)
                }
                Label { Layout.fillWidth: true; text: (page.edition.players || "1")+"P · "+(page.edition.controls || "")+" · "+(page.edition.category || ""); visible: text.length>0; color: "#91a3b3"; textFormat: Text.PlainText }
                Label { Layout.fillWidth: true; text: page.edition.playable ? (page.edition.status === "preliminary" ? qsTranslate("Vetust", "ROMs verified · Emulation not working") : page.edition.status === "imperfect" ? qsTranslate("Vetust", "ROMs verified · Emulation has limitations") : qsTranslate("Vetust", "ROMs verified")) : qsTranslate("Vetust", "Missing or invalid ROMs"); visible: text.length>0; wrapMode: Text.WordWrap; textFormat: Text.PlainText; color: "#91a3b3" }
                RowLayout {
                    Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; objectName: "favoriteArcadeGame"; text: page.selected.favorite ? "♥" : "♡"; Accessible.name: qsTranslate("Vetust", "FAVORITES"); onClicked: vetust.arcade.setFavorite(page.selected.id,!page.selected.favorite) }
                    Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; objectName: "hideArcadeGame"; text: page.selected.hidden ? qsTranslate("Vetust", "Unhide game") : qsTranslate("Vetust", "Hide game"); onClicked: {vetust.arcade.setHidden(page.selected.id,!page.selected.hidden);details.close()} }
                }
                Label { Layout.fillWidth:true; wrapMode:Text.WordWrap; color:"#e9bd79"; visible:page.edition.audit==="best available"; text:qsTranslate("Vetust", "Best available dump · Some data has not been recovered") }
                ScrollView {
                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 85
                    clip: true
                    TextArea { text: page.edition.history || ""; readOnly: true; wrapMode: TextEdit.Wrap; textFormat: TextEdit.PlainText; color: "#91a3b3"; background: null; selectByMouse: true }
                }
                Label { Layout.fillWidth:true; wrapMode:Text.WordWrap; color:"#91a3b3"; text: page.edition.savestate ? qsTranslate("Vetust", "Save states supported") : qsTranslate("Vetust", "Save states are not supported by this machine") }

                Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; id: playArcadeButton; objectName: "playArcade"; text: qsTranslate("Vetust", "Play in MAME 0.241 →"); enabled: !!page.edition.id && !!page.edition.playable && !vetust.busy && vetust.arcade.available; onClicked: {vetust.arcade.play(page.selected.id,page.edition.id);details.close()} }
                Label { Layout.fillWidth: true; text: qsTranslate("Vetust", "5: insert coin · 1: start · Tab: controls · Esc: exit. Shift+F7: save · F7: load (when supported)."); color: "#91a3b3"; wrapMode: Text.WordWrap }
            }
        }
    }
    Dialog {
        id: options; objectName: "arcadeOptions"
        property url folder: ""
        anchors.centerIn: Overlay.overlay; width: Math.min(750,Overlay.overlay.width-60)
        title: qsTranslate("Vetust", "Arcade settings"); modal: true; focus: true; standardButtons: Dialog.NoButton
        palette.window: "#101c27"; palette.windowText: "#e7edf1"; palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; palette.base: "#1b2d3b"; palette.text: "#e7edf1"
        onOpened: { folder=vetust.arcade.settings.folderUrl; fullscreen.checked=vetust.arcade.settings.fullscreen; artwork.checked=vetust.arcade.settings.artwork; error.text="" }
        contentItem: ColumnLayout {
            spacing: 14
            Label { text: qsTranslate("Vetust", "Arcade library folder"); color: "#e7edf1" }
            Label { Layout.fillWidth: true; text: options.folder.toString() || vetust.arcade.settings.resolvedFolder; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; color: "#91a3b3" }
            RowLayout {
                Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; text: qsTranslate("Vetust", "Choose folder…"); onClicked: folderPicker.open() }
                Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; text: qsTranslate("Vetust", "Use the main library"); onClicked: options.folder="" }
            }
            Switch { id: fullscreen; text: qsTranslate("Vetust", "Fullscreen") }
            Switch { id: artwork; text: qsTranslate("Vetust", "Show cabinet artwork around the game") }
            Label { Layout.fillWidth:true; wrapMode:Text.WordWrap; color:"#e9bd79"; text:qsTranslate("Vetust", "MAME is pinned to 0.241 for this ROM set. It is not updated automatically.") }
            Label { Layout.fillWidth: true; text: qsTranslate("Vetust", "Uses the monitor selected in global settings. New saves stay in this installation of VETUSTware."); color: "#91a3b3"; wrapMode: Text.WordWrap }
            Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; text: qsTranslate("Vetust", "Manage in MAME 0.241"); enabled: !vetust.busy && vetust.arcade.available; onClicked: {error.text=vetust.arcade.configure(options.folder,fullscreen.checked,artwork.checked);if(!error.text){vetust.arcade.manage();options.close()}} }
            Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; text: qsTranslate("Vetust", "Open saves and settings"); onClicked: vetust.arcade.openFolder(true) }
            Label { id: error; Layout.fillWidth: true; visible: text.length>0; color: "#f1a292"; wrapMode: Text.WordWrap }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; text: qsTranslate("Vetust", "Cancel"); onClicked: options.close() }
                Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; text: qsTranslate("Vetust", "Save"); onClicked: {error.text=vetust.arcade.configure(options.folder,fullscreen.checked,artwork.checked);if(!error.text){options.close();vetust.arcade.enter()}} }
            }
        }
    }
    FolderDialog { id: folderPicker; title: qsTranslate("Vetust", "Arcade library folder"); onAccepted: options.folder=selectedFolder }
}
