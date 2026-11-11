import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: page
    objectName: "scummView"
    property string query: ""
    property bool showHidden: false
    property bool detailsOpen: details.visible
    property var selected: ({})
    readonly property string metadataGameId: (variants.find(v=>v.target===target) || {}).gameId || selected.gameId || ""
    property var metadata: {
        languages.current
        const local=vetust.scumm.gameMetadata(selected.folder || "",metadataGameId)
        return Object.keys(local).length ? local : vetust.scummArtwork(metadataGameId)
    }
    property int shotIndex: 0
    readonly property var shots: metadata.shots || []
    readonly property string cover: selected.cover || media(metadata.cover)
    readonly property string shot: shots.length ? media(shots[Math.min(shotIndex,shots.length-1)]) : ""
    function media(ref) { return !ref ? "" : ref.startsWith("file:") || ref.startsWith("image:") ? ref : "image://art/"+ref }
    onMetadataChanged: shotIndex=0
    property var variants: selected.variants || []
    property string target: selected.target || ""
    spacing: 16
    function showGame(entry) { selected=entry; target=entry.target || ""; shotIndex=0; details.open() }
    Connections {
        target: vetust.scumm
        function onChanged() {
            if(page.selected.folder) {
                const updated=vetust.scumm.games.find(g=>g.folder===page.selected.folder)
                if(updated){page.selected=updated;page.target=updated.target || ""}
            }
        }
    }
    RowLayout {
        Layout.fillWidth: true; spacing: 14
        ColumnLayout {
            id: heading
            Layout.fillWidth: true; spacing: 5
            Label { text: "SCUMMVM"; color: "#78d8d1"; font.pixelSize: 11; font.letterSpacing: 2 }
            Label { text: qsTranslate("Vetust", "Adventures, another way"); color: "#e9bd79"; font.pixelSize: 28; font.family: "Noto Serif" }
            Label { Layout.fillWidth: true; text: qsTranslate("Vetust", "Your ScummVM editions, with their own settings and saves."); color: "#91a3b3"; wrapMode: Text.WordWrap }
        }
        Image {
            objectName: "scummHeaderArt"
            property real availableWidth: page.width-heading.implicitWidth-headerActions.implicitWidth-48
            visible: availableWidth >= 160
            Layout.preferredWidth: Math.min(280, Math.max(0, availableWidth))
            Layout.preferredHeight: Layout.preferredWidth*9/16
            Layout.alignment: Qt.AlignVCenter
            source: "assets/scummvm-header.png"
            sourceSize.width: 720
            fillMode: Image.PreserveAspectFit
            Accessible.ignored: true
        }
        RowLayout {
            id: headerActions
            spacing: 12
            Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; objectName: "scanScummVM"; text: qsTranslate("Vetust", "Update catalog"); enabled: !vetust.busy; onClicked: vetust.scumm.scan() }
            Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; objectName: "manageScummVM"; text: qsTranslate("Vetust", "Manage in ScummVM"); enabled: !vetust.busy && vetust.scumm.available; onClicked: vetust.scumm.manage() }
            ToolButton { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; text: "↗"; Accessible.name: qsTranslate("Vetust", "Open library folder"); ToolTip.text: Accessible.name; ToolTip.visible: hovered; onClicked: vetust.scumm.openFolder() }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Button {
            objectName: "showHiddenScummVM"
            text: qsTranslate("Vetust", "Show hidden")
            checkable: true; checked: page.showHidden
            onClicked: page.showHidden=checked
            palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"
        }
        Label { visible: page.showHidden; text: qsTranslate("Vetust", "Hidden games are included."); color: "#91a3b3"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
    }
    Label { Layout.fillWidth: true; visible: text.length>0; text: vetust.scumm.status; color: "#78d8d1"; wrapMode: Text.WordWrap; textFormat: Text.PlainText }
    GridView {
        id: grid; objectName: "scummGrid"
        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
        cellWidth: Math.floor(width/Math.max(3,Math.floor(width/205))); cellHeight: 330
        model: vetust.scumm.games.filter(g=>(page.showHidden || !g.hidden) && (g.title+" "+g.folder+" "+g.variants.map(v=>v.title).join(" ")).toLocaleLowerCase().includes(page.query.toLocaleLowerCase()))
        boundsBehavior: Flickable.StopAtBounds; cacheBuffer: 350
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        delegate: Button {
            id: tile
            required property var modelData
            required property int index
            property var art: vetust.scummArtwork(modelData.gameId || "")
            width: grid.cellWidth-16; height: grid.cellHeight-18; padding: 10; hoverEnabled: true
            objectName: "scummTile-"+index
            Accessible.name: modelData.title+" · "+modelData.folder
            onClicked: page.showGame(modelData)
            onActiveFocusChanged: if(activeFocus)grid.currentIndex=index
            Keys.onRightPressed: {grid.moveCurrentIndexRight();if(grid.currentItem)grid.currentItem.forceActiveFocus()}
            Keys.onLeftPressed: {grid.moveCurrentIndexLeft();if(grid.currentItem)grid.currentItem.forceActiveFocus()}
            Keys.onDownPressed: {grid.moveCurrentIndexDown();if(grid.currentItem)grid.currentItem.forceActiveFocus()}
            Keys.onUpPressed: {grid.moveCurrentIndexUp();if(grid.currentItem)grid.currentItem.forceActiveFocus()}
            Keys.onReturnPressed: clicked()
            background: Rectangle { radius: 7; color: tile.hovered ? "#1b2b38" : "#101c27"; border.color: tile.activeFocus ? "#78d8d1" : tile.hovered ? "#e9bd79" : "#2d3e4b" }
            contentItem: ColumnLayout {
                spacing: 6
                Rectangle {
                    Layout.fillWidth: true; Layout.fillHeight: true; color: "#080f17"; radius: 4
                    Rectangle {
                        anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 7
                        z: 1; visible: !!tile.modelData.hidden; color: "#101c27"; radius: 3
                        width: hiddenBadge.implicitWidth+16; height: hiddenBadge.implicitHeight+8
                        Label { id: hiddenBadge; anchors.centerIn: parent; text: qsTranslate("Vetust", "Hidden"); color: "#e9bd79"; font.pixelSize: 11 }
                    }
                    Image { anchors.fill: parent; anchors.margins: 3; source: tile.modelData.cover || (tile.art.cover ? "image://art/"+tile.art.cover : ""); fillMode: Image.PreserveAspectFit; asynchronous: true; sourceSize.width: 300 }
                    Label { anchors.centerIn: parent; width: parent.width-18; visible: !tile.modelData.cover && !tile.art.cover; text: tile.modelData.title; textFormat: Text.PlainText; color: "#e9bd79"; font.pixelSize: 20; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap }
                }
                Label { Layout.fillWidth: true; Layout.preferredHeight: 34; text: tile.modelData.title; textFormat: Text.PlainText; color: "#e7edf1"; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight; font.pixelSize: 12 }
                Label { Layout.fillWidth: true; text: tile.modelData.folder; textFormat: Text.PlainText; color: "#91a3b3"; elide: Text.ElideRight; font.pixelSize: 10 }
                Label { Layout.fillWidth: true; text: !tile.modelData.variants.length ? qsTranslate("Vetust", "Not detected") : !tile.modelData.target ? qsTranslate("Vetust", "Choose an edition") : qsTranslate("Vetust", "Play in ScummVM →"); color: tile.modelData.target ? "#78d8d1" : "#e9bd79"; elide: Text.ElideRight; font.pixelSize: 11 }
            }
        }
        Label { anchors.centerIn: parent; width: Math.min(600,parent.width-40); visible: grid.count===0; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; color: "#91a3b3"; text: page.query ? qsTranslate("Vetust", "No games match your search.") : vetust.scumm.games.length>0 ? qsTranslate("Vetust", "All games are hidden. Use Show hidden to see them.") : qsTranslate("Vetust", "Place each game in its own folder inside scummvm, then update the catalog.") }
    }
    Dialog {
        id: details; objectName: "scummDetails"
        background: LibraryBackdrop { source: page.media(page.metadata.background) || page.cover }
        parent: Overlay.overlay; x: 0; y: 0; width: parent.width; height: parent.height; margins: 0
        modal: true; focus: true; title: page.selected.title || "ScummVM"
        palette.window: "#101c27"; palette.windowText: "#e9bd79"; palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"
        standardButtons: Dialog.NoButton
        footer: null
        header: Item {
            implicitHeight: 56
            RowLayout {
                anchors.fill: parent; anchors.margins: 12
                Label { Layout.fillWidth: true; text: details.title; textFormat: Text.PlainText; color: "#e9bd79"; font.pixelSize: 14; font.bold: true; elide: Text.ElideRight }
                ToolButton {
                    objectName: "closeScummDetailsButton"; text: "×"; font.pixelSize: 26
                    implicitWidth: 36; implicitHeight: 32
                    Accessible.name: qsTranslate("Vetust", "Close")
                    ToolTip.text: qsTranslate("Vetust", "Close (Esc)"); ToolTip.visible: hovered
                    onClicked: details.close()
                }
            }
        }
        contentItem: RowLayout {
            spacing: details.width<1200 ? 20 : 32
            ColumnLayout {
                Layout.preferredWidth: Math.min(260,details.width*0.18); Layout.fillHeight: true
                Item { Layout.fillHeight: true }
                Image {
                    Layout.fillWidth: true; Layout.preferredHeight: Math.min(420,details.height-170)
                    source: page.media(page.metadata.box3d) || page.cover
                    fillMode: Image.PreserveAspectFit; asynchronous: true; sourceSize.width: 650
                    MouseArea {
                        objectName: "scummDetailPlayImage"
                        anchors.fill: parent; enabled: playScummButton.enabled
                        cursorShape: Qt.PointingHandCursor; hoverEnabled: true
                        ToolTip.text: playScummButton.text; ToolTip.visible: containsMouse && enabled
                        onClicked: playScummButton.clicked()
                    }
                }
                Item { Layout.fillHeight: true }
            }
            ColumnLayout {
                Layout.fillWidth: true; Layout.preferredWidth: 430; Layout.minimumWidth: 290; spacing: 12
                Label { text: "SCUMMVM"+(page.metadata.year ? "     —     "+page.metadata.year : ""); color: "#78d8d1"; font.pixelSize: 11; font.letterSpacing: 2 }
                Label { Layout.fillWidth: true; text: page.metadata.title || page.selected.title || ""; textFormat: Text.PlainText; color: "#f1d3a0"; font.family: "Noto Serif"; font.pixelSize: details.width<1200 ? 27 : 34; wrapMode: Text.WordWrap; maximumLineCount: 3; elide: Text.ElideRight }
                Label { Layout.fillWidth: true; visible: !!text; text: page.metadata.developer || page.metadata.publisher || ""; color: "#c2cbd2"; textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                Label { Layout.fillWidth: true; visible: !!text; text: (page.metadata.genre || "").split(";").map(g=>qsTranslate("Vetust", g)).join(" · "); color: "#91a3b3"; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.pixelSize: 12 }
                Label { Layout.fillWidth: true; text: page.metadata.description || qsTranslate("Vetust", "This edition runs directly in ScummVM."); textFormat: Text.PlainText; wrapMode: Text.WordWrap; maximumLineCount: details.height<800 ? 4 : 6; elide: Text.ElideRight; color: "#a8bac8"; font.pixelSize: 13; lineHeight: 1.25 }
                Button { objectName: "scummFullDescription"; visible: !!page.metadata.description; text: qsTranslate("Vetust", "Read full details ↗"); flat: true; palette.buttonText: "#e9bd79"; onClicked: descriptionDialog.open() }
                Label { Layout.fillWidth: true; visible: page.variants.length>1; text: qsTranslate("Vetust", "ScummVM found several possible editions. Choose the one that matches your files."); color: "#e9bd79"; font.pixelSize: 12; wrapMode: Text.WordWrap }
                ComboBox {
                    id: choice; objectName: "scummEdition"
                    Layout.fillWidth: true; visible: page.variants.length>0; enabled: !vetust.busy
                    model: page.variants; textRole: "title"; valueRole: "target"
                    currentIndex: page.variants.findIndex(v=>v.target===page.target)
                    displayText: currentIndex<0 ? qsTranslate("Vetust", "Choose an edition") : currentText
                    onActivated: {page.target=currentValue;vetust.scumm.choose(currentValue)}
                }
                Label { visible: !page.variants.length; Layout.fillWidth: true; text: qsTranslate("Vetust", "ScummVM did not recognize this folder. Check its files or use the ScummVM manager."); wrapMode: Text.WordWrap; color: "#e9bd79" }
                Flow {
                    Layout.fillWidth: true; spacing: 8
                    Button { id: playScummButton; objectName: "playScummVM"; text: qsTranslate("Vetust", "Play in ScummVM →"); highlighted: true; palette.highlight: "#78d8d1"; palette.highlightedText: "#101c27"; enabled: !!page.target && !vetust.busy && vetust.scumm.available; onClicked: {vetust.scumm.play(page.target);details.close()} }
                    Button { objectName: "scummManual"; text: qsTranslate("Vetust", "▤  Manual"); visible: !!page.selected.manual; enabled: !vetust.busy; onClicked: vetust.scumm.openManual(page.selected.folder) }
                    Button { text: qsTranslate("Vetust", "Open folder"); onClicked: vetust.scumm.openFolder(page.selected.folder) }
                    Button { objectName: "hideScummGame"; text: page.selected.hidden ? qsTranslate("Vetust", "Unhide game") : qsTranslate("Vetust", "Hide game"); enabled: !vetust.busy; onClicked: {vetust.scumm.setHidden(page.selected.folder,!page.selected.hidden);details.close()} }
                }
                Label { Layout.fillWidth: true; text: qsTranslate("Vetust", "Use Manage in ScummVM to change graphics, sound and game options."); color: "#91a3b3"; wrapMode: Text.WordWrap; font.pixelSize: 11 }
            }
            ColumnLayout {
                visible: page.shots.length>0; Layout.preferredWidth: Math.min(details.width*0.36,620); spacing: 14
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: width*0.75; color: "#03070a"; radius: 6; border.color: "#34404b"
                    Image { objectName: "scummScreenshot"; anchors.fill: parent; anchors.margins: 6; source: page.shot; fillMode: Image.PreserveAspectFit; asynchronous: true; sourceSize.width: 1200 }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: galleryDialog.open() }
                    Label { anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 10; text: "⛶"; color: "#b9cbd6" }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: (page.shots.length ? page.shotIndex+1 : 0)+" / "+page.shots.length; color: "#91a3b3"; font.pixelSize: 11 }
                    Item { Layout.fillWidth: true }
                    Repeater { model: page.shots.slice(0,4)
                        delegate: Rectangle {
                            required property string modelData; required property int index
                            width: 52; height: 40; color: "#0b151e"; radius: 3; border.color: page.shotIndex===index ? "#e9bd79" : "#34404b"
                            Image { anchors.fill: parent; anchors.margins: 3; source: page.media(modelData); fillMode: Image.PreserveAspectFit; asynchronous: true; sourceSize.width: 180 }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: page.shotIndex=index }
                        }
                    }
                    ToolButton { visible: page.shots.length>1; text: "→"; onClicked: page.shotIndex=(page.shotIndex+1)%page.shots.length }
                }
                Label { Layout.fillWidth: true; visible: !!page.metadata.referenceImages; text: qsTranslate("Vetust", "Reference images · DOS edition"); color: "#91a3b3"; font.pixelSize: 10; wrapMode: Text.WordWrap }
            }
        }
    }
    Dialog {
        id: descriptionDialog; objectName: "scummDescriptionDialog"
        parent: Overlay.overlay; anchors.centerIn: parent; width: Math.min(850,parent.width-40); height: Math.min(700,parent.height-40); modal: true; focus: true
        palette.window: "#101c27"; palette.windowText: "#e7edf1"; background: Rectangle { color: "#101c27"; border.color: "#34404b" }
        header: RowLayout {
            Label { Layout.fillWidth: true; Layout.margins: 14; text: page.metadata.title || page.selected.title || ""; textFormat: Text.PlainText; color: "#e9bd79"; elide: Text.ElideRight }
            ToolButton { text: "×"; font.pixelSize: 26; Accessible.name: qsTranslate("Vetust", "Close"); onClicked: descriptionDialog.close() }
        }
        contentItem: ScrollView { id: descriptionScroll; clip: true; contentWidth: availableWidth
            TextArea { width: descriptionScroll.availableWidth; text: page.metadata.description || ""; textFormat: TextEdit.PlainText; wrapMode: TextEdit.Wrap; readOnly: true; selectByMouse: true; color: "#d3dde5"; font.pixelSize: 15 }
        }
    }
    Dialog {
        id: galleryDialog; objectName: "scummGalleryDialog"
        parent: Overlay.overlay; x: 0; y: 0; width: parent.width; height: parent.height; margins: 0; modal: true; focus: true
        background: Rectangle { color: "#080f17" }
        header: RowLayout {
            ToolButton { text: "←"; enabled: page.shotIndex>0; onClicked: page.shotIndex-- }
            Label { text: (page.shotIndex+1)+" / "+page.shots.length; color: "#e7edf1" }
            ToolButton { text: "→"; enabled: page.shotIndex<page.shots.length-1; onClicked: page.shotIndex++ }
            Item { Layout.fillWidth: true }
            ToolButton { text: "×"; font.pixelSize: 26; Accessible.name: qsTranslate("Vetust", "Close"); onClicked: galleryDialog.close() }
        }
        contentItem: Image { source: page.shot; fillMode: Image.PreserveAspectFit; asynchronous: true; sourceSize.width: 1800 }
    }
}
