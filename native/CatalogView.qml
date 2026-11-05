import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: catalog
    objectName: "unifiedCatalog"
    property string query: ""
    property var result: ({games:[], count:0, page:0, pages:1})
    signal openGame(string engine, string id)
    spacing: 16
    function engineName(engine) { return ({dosbox:"DOSBox",scummvm:"ScummVM",atari:"Atari 2600",arcade:"Arcade · MAME"})[engine] || engine }
    function refresh(page) {
        result=vetust.unifiedCatalog(query,engineFilter.currentValue || "",page,genre.currentText || "",verified.checked)
        grid.positionViewAtBeginning()
    }
    function reset() { refresh(0) }
    onQueryChanged: debounce.restart()
    Component.onCompleted: reset()
    Timer { id: debounce; interval: 180; onTriggered: catalog.reset() }
    Connections { target: vetust; function onChanged() { catalog.refresh(catalog.result.page) } }
    Connections { target: vetust.scumm; function onChanged() { catalog.refresh(catalog.result.page) } }
    Connections { target: vetust.atari; function onChanged() { catalog.refresh(catalog.result.page) } }
    Connections { target: vetust.arcade; function onChanged() { catalog.refresh(catalog.result.page) } }
    ColumnLayout {
        Layout.fillWidth: true; spacing: 6
        Label { text: catalog.query ? qsTranslate("Vetust", "SEARCH RESULTS") : qsTranslate("Vetust", "EXPLORE AT YOUR OWN PACE"); color: "#78d8d1"; font.pixelSize: 9; font.letterSpacing: 1.5 }
        Label { objectName: "unifiedCatalogCount"; Layout.fillWidth: true; text: qsTranslate("Vetust", "Full catalog")+" · "+catalog.result.count.toLocaleString(Qt.locale(languages.current),"f",0); color: "#e7edf1"; font.pixelSize: 21 }
    }
    RowLayout {
        Layout.fillWidth: true; spacing: 12
        ComboBox {
            id: engineFilter; objectName: "catalogEngineFilter"
            implicitWidth: 190; implicitHeight: 34
            textRole: "label"; valueRole: "value"
            model: [{label:qsTranslate("Vetust", "All engines"),value:""},{label:"DOSBox",value:"dosbox"},{label:"ScummVM",value:"scummvm"},{label:"Atari 2600",value:"atari"},{label:"Arcade · MAME",value:"arcade"}]
            palette.button: "#121e28"; palette.buttonText: "#e7edf1"; palette.text: "#e7edf1"; palette.base: "#121e28"
            onActivated: { genre.currentIndex=0; verified.checked=false; catalog.reset() }
        }
        ComboBox {
            id: genre; objectName: "unifiedGenreFilter"
            visible: engineFilter.currentValue === "dosbox"
            model: vetust.genres; implicitWidth: 190; implicitHeight: 34
            displayText: window.genreName(currentText)
            delegate: ItemDelegate { required property string modelData; width: genre.width; text: window.genreName(modelData); palette.text: "#e7edf1" }
            palette.button: "#121e28"; palette.buttonText: "#e7edf1"; palette.text: "#e7edf1"; palette.base: "#121e28"
            onActivated: catalog.reset()
        }
        CheckBox { id: verified; visible: engineFilter.currentValue === "dosbox"; text: qsTranslate("Vetust", "Verified"); palette.windowText: "#e7edf1"; onClicked: catalog.reset(); ToolTip.text: qsTranslate("Vetust", "Show only games whose startup has been verified"); ToolTip.visible: hovered }
        Item { Layout.fillWidth: true }
        Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; objectName: "unifiedPrevious"; text: "←"; implicitWidth: 36; implicitHeight: 34; enabled: catalog.result.page>0; onClicked: catalog.refresh(catalog.result.page-1) }
        Label { text: (catalog.result.page+1)+" / "+catalog.result.pages; color: "#91a3b3"; font.pixelSize: 11 }
        Button { palette.button: "#1b2d3b"; palette.buttonText: "#e7edf1"; objectName: "unifiedNext"; text: "→"; implicitWidth: 36; implicitHeight: 34; enabled: catalog.result.page<catalog.result.pages-1; onClicked: catalog.refresh(catalog.result.page+1) }
    }
    GridView {
        id: grid; objectName: "unifiedCatalogGrid"
        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
        model: catalog.result.games
        cellWidth: Math.floor(width/Math.max(4,Math.floor(width/180))); cellHeight: 310
        boundsBehavior: Flickable.StopAtBounds; cacheBuffer: 310
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        delegate: ItemDelegate {
            id: tile
            required property var modelData
            required property int index
            objectName: "catalogTile-"+modelData.key
            width: grid.cellWidth-16; height: grid.cellHeight-16
            Accessible.name: modelData.title+" · "+catalog.engineName(modelData.engine)
            onClicked: catalog.openGame(modelData.engine,modelData.id)
            onActiveFocusChanged: if(activeFocus)grid.currentIndex=index
            Keys.onReturnPressed: clicked()
            Keys.onEnterPressed: clicked()
            Keys.onRightPressed: {grid.moveCurrentIndexRight();if(grid.currentItem)grid.currentItem.forceActiveFocus()}
            Keys.onLeftPressed: {grid.moveCurrentIndexLeft();if(grid.currentItem)grid.currentItem.forceActiveFocus()}
            Keys.onDownPressed: {grid.moveCurrentIndexDown();if(grid.currentItem)grid.currentItem.forceActiveFocus()}
            Keys.onUpPressed: {grid.moveCurrentIndexUp();if(grid.currentItem)grid.currentItem.forceActiveFocus()}
            background: Rectangle { radius: 7; color: "#101d27"; border.color: tile.hovered || tile.activeFocus ? "#e9bd79" : "#304553" }
            contentItem: ColumnLayout {
                spacing: 7
                Rectangle {
                    Layout.fillWidth: true; Layout.fillHeight: true; color: "#080f17"; radius: 4
                    Image { id: cover; anchors.fill: parent; anchors.margins: 5; source: tile.modelData.cover || ""; fillMode: Image.PreserveAspectFit; asynchronous: true; sourceSize.width: 380 }
                    Label { anchors.centerIn: parent; width: parent.width-20; visible: !cover.source.toString() || cover.status===Image.Error; text: tile.modelData.title; color: "#91a3b3"; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap }
                }
                Label { Layout.fillWidth: true; text: tile.modelData.title; color: "#e7edf1"; font.pixelSize: 12; maximumLineCount: 2; wrapMode: Text.WordWrap; elide: Text.ElideRight; Layout.preferredHeight: 32 }
                Label { Layout.fillWidth: true; text: catalog.engineName(tile.modelData.engine); color: "#78d8d1"; font.pixelSize: 11; elide: Text.ElideRight }
                Label { Layout.fillWidth: true; text: [tile.modelData.year,tile.modelData.developer].filter(v=>v).join(" · "); color: "#91a3b3"; font.pixelSize: 10; elide: Text.ElideRight }
            }
        }
        Label { anchors.centerIn: parent; visible: catalog.result.count===0; text: qsTranslate("Vetust", "No games match your search."); color: "#91a3b3" }
    }
}
