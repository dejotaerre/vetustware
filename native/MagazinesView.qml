import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: page
    objectName: "magazinesView"
    signal navigationChanged()
    property string query: ""
    property var collection: ({})
    property var issues: []
    property var series: vetust.magazineSeries
    property bool inCollection: !!collection.id
    property bool dos: inCollection && collection.kind === "dos"
    spacing: 16
    function reset() { collection={}; issues=[]; grid.positionViewAtBeginning(); navigationChanged() }
    function selectCollection(item) { collection=item; issues=vetust.magazineIssues(item.id); grid.positionViewAtBeginning(); navigationChanged() }
    onSeriesChanged: reset()
    RowLayout {
        Layout.fillWidth: true; spacing: 16
        Button { visible: page.inCollection; text: qsTranslate("Vetust", "← Collections"); onClicked: page.reset() }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 6
            Label { text: qsTranslate("Vetust", "THE MAGAZINE LIBRARY"); color: "#78d8d1"; font.pixelSize: 10; font.letterSpacing: 2 }
            Label { Layout.fillWidth: true; text: page.inCollection ? page.collection.title : qsTranslate("Vetust", "Magazines"); color: "#e9bd79"; font.pixelSize: 30; font.family: "Noto Serif"; elide: Text.ElideRight; textFormat: Text.PlainText }
            Label { Layout.fillWidth: true; text: page.inCollection ? page.collection.description : qsTranslate("Vetust", "Reviews, stories and advertising from the era. Open a collection and explore its pages."); color: "#91a3b3"; wrapMode: Text.WordWrap; textFormat: Text.PlainText }
        }
        Button { visible: !!page.collection.indexId; text: qsTranslate("Vetust", "Original index"); enabled: !vetust.busy; onClicked: vetust.openMagazine(page.collection.indexId) }
    }
    Rectangle {
        visible: page.dos
        Layout.fillWidth: true; Layout.fillHeight: true
        color: "#101e2b"; radius: 9; border.color: "#344552"
        ColumnLayout {
            anchors.centerIn: parent; width: Math.min(580,parent.width-60); spacing: 22
            Label { text: "A:\\>_"; color: "#78d8d1"; font.family: "monospace"; font.pixelSize: 54 }
            Label { Layout.fillWidth: true; text: qsTranslate("Vetust", "A magazine you can run"); color: "#e7edf1"; font.pixelSize: 24 }
            Label { Layout.fillWidth: true; text: qsTranslate("Vetust", "Browse the issues in their original DOS menu. Changes are saved on this computer; the library's original files remain intact."); color: "#a8bac8"; wrapMode: Text.WordWrap; font.pixelSize: 15 }
            Button { objectName: "openDosMagazine"; text: qsTranslate("Vetust", "Open in DOSBox →"); enabled: !vetust.busy; onClicked: vetust.openMagazine(page.collection.launchId) }
        }
    }
    GridView {
        id: grid; objectName: "magazinesGrid"
        visible: !page.dos
        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
        cellWidth: Math.floor(width/Math.max(3,Math.floor(width/220))); cellHeight: page.inCollection ? 342 : 382
        cacheBuffer: 400; boundsBehavior: Flickable.StopAtBounds
        model: (page.inCollection ? page.issues : page.series).filter(item => (item.title + " " + (item.description || "")).toLocaleLowerCase().includes(page.query.toLocaleLowerCase()))
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        delegate: Button {
            id: tile
            required property var modelData
            required property int index
            objectName: "magazineTile-"+modelData.id
            width: grid.cellWidth-18; height: grid.cellHeight-20
            padding: 10; hoverEnabled: true
            enabled: !vetust.busy || !page.inCollection
            Accessible.name: modelData.title
            ToolTip.text: modelData.title; ToolTip.visible: hovered; ToolTip.delay: 600
            onClicked: page.inCollection ? vetust.openMagazine(modelData.id) : page.selectCollection(modelData)
            onActiveFocusChanged: if(activeFocus)grid.currentIndex=index
            Keys.onRightPressed: {grid.moveCurrentIndexRight();if(grid.currentItem)grid.currentItem.forceActiveFocus()}
            Keys.onLeftPressed: {grid.moveCurrentIndexLeft();if(grid.currentItem)grid.currentItem.forceActiveFocus()}
            Keys.onDownPressed: {grid.moveCurrentIndexDown();if(grid.currentItem)grid.currentItem.forceActiveFocus()}
            Keys.onUpPressed: {grid.moveCurrentIndexUp();if(grid.currentItem)grid.currentItem.forceActiveFocus()}
            Keys.onReturnPressed: clicked()
            background: Rectangle { radius: 7; color: tile.hovered ? "#1b2b38" : "#101c27"; border.color: tile.activeFocus ? "#78d8d1" : tile.hovered ? "#e9bd79" : "#2d3e4b"; border.width: tile.activeFocus ? 2 : 1 }
            contentItem: ColumnLayout {
                spacing: 10
                Rectangle {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    color: "#080f17"; radius: 3
                    Image { anchors.fill: parent; anchors.margins: 3; source: tile.modelData.cover || ""; asynchronous: true; fillMode: Image.PreserveAspectFit; sourceSize.width: 460 }
                    Column {
                        visible: tile.modelData.kind === "dos"
                        anchors.centerIn: parent; spacing: 18
                        Label { anchors.horizontalCenter: parent.horizontalCenter; text: "A:\\>_"; color: "#78d8d1"; font.pixelSize: 36; font.family: "monospace" }
                        Label { anchors.horizontalCenter: parent.horizontalCenter; text: tile.modelData.id === "BBD" ? "BIG BLUE\nDISK" : "GAME\nBYTES"; horizontalAlignment: Text.AlignHCenter; color: "#e9bd79"; font.pixelSize: 28; font.bold: true }
                        Label { anchors.horizontalCenter: parent.horizontalCenter; text: qsTranslate("Vetust", "DISK MAGAZINE"); color: "#91a3b3"; font.pixelSize: 9; font.letterSpacing: 1 }
                    }
                }
                Label { Layout.fillWidth: true; Layout.preferredHeight: 36; text: tile.modelData.title; color: "#e7edf1"; font.pixelSize: 14; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight; textFormat: Text.PlainText }
                Label { Layout.fillWidth: true; text: page.inCollection ? qsTranslate("Vetust", "Read magazine  →") : (tile.modelData.kind === "dos" ? qsTranslate("Vetust", "Issues · %1 · DOS") : qsTranslate("Vetust", "Issues · %1 · PDF")).arg(tile.modelData.count); color: "#78d8d1"; font.pixelSize: 11 }
            }
        }
        Label {
            anchors.centerIn: parent; width: Math.min(620,parent.width-40)
            visible: grid.count===0; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; color: "#91a3b3"; font.pixelSize: 16
            text: vetust.magazineError || (page.series.length===0 ? qsTranslate("Vetust", "This library does not include magazines.\nConnect the drive containing the Revistas folder.") : qsTranslate("Vetust", "No magazines match your search."))
        }
    }
}
