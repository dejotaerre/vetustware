import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Pdf

ApplicationWindow {
    id: window
    visible: true
    width: 1480; height: 920
    minimumWidth: 1050; minimumHeight: 720
    title: "VETUSTware · Old School Gamer"
    color: "#080f17"
    property color gold: "#e9bd79"
    property color cyan: "#78d8d1"
    property color ink: "#e7edf1"
    property color muted: "#91a3b3"
    property var game: vetust.selected
    property string view: "home"
    property string collectionKey: ""
    property string collectionTitle: ""
    readonly property var sections: [{label:qsTranslate("Vetust", "HOME"),value:"home"},{label:qsTranslate("Vetust", "CATALOG"),value:"all"},{label:qsTranslate("Vetust", "FAVORITES"),value:"favorites"},{label:qsTranslate("Vetust", "RECENT"),value:"recent"},{label:"SCUMMVM",value:"scummvm"},{label:"ATARI 2600",value:"atari"},{label:"ARCADE",value:"arcade"},{label:qsTranslate("Vetust", "MAGAZINES"),value:"magazines"}]
    readonly property int spotlightColumns: width >= 2300 ? 3 : width >= 1700 ? 2 : 1
    onSpotlightColumnsChanged: vetust.refreshHome(false,spotlightColumns)
    property int artRevision: 0
    property bool modalOpen: (arcadeLoader.item ? arcadeLoader.item.detailsOpen : false) || backgroundsDialog.visible || runtimesDialog.visible || storageDialog.visible || details.visible || emulatorDialog.visible || infoDialog.visible || imageDialog.visible || manualDialog.visible || extrasDialog.visible || busyDialog.visible || (scummLoader.item ? scummLoader.item.detailsOpen : false) || (atariLoader.item ? atariLoader.item.detailsOpen : false)
    property string catalogDetailsEngine: ""
    readonly property bool unifiedCatalogVisible: view === "all" && collectionKey === ""
    property bool onlyTested: false
    property int shotIndex: 0
    property var shots: game.shots || []
    property string shot: shots.length ? shots[Math.min(shotIndex,shots.length-1)] : (game.cover || "")
    function genreName(value) {
        const names = {Adventure:qsTranslate("Vetust", "Adventure"),Action:qsTranslate("Vetust", "Action"),Platform:qsTranslate("Vetust", "Platformers"),Puzzle:qsTranslate("Vetust", "Puzzles"),Strategy:qsTranslate("Vetust", "Strategy"),Sports:qsTranslate("Vetust", "Sports"),Simulation:qsTranslate("Vetust", "Simulation"),Shooter:qsTranslate("Vetust", "Shooters"),Education:qsTranslate("Vetust", "Educational"),"Board / Party Game":qsTranslate("Vetust", "Board and party games"),"Construction and Management Simulation":qsTranslate("Vetust", "Construction and management"),Fighting:qsTranslate("Vetust", "Fighting"),"First Person Shooter":qsTranslate("Vetust", "First-person shooters"),"Flight Simulator":qsTranslate("Vetust", "Flight simulation"),"Game Show":qsTranslate("Vetust", "Game shows"),"Interactive Fiction":qsTranslate("Vetust", "Interactive fiction"),"Interactive Movie":qsTranslate("Vetust", "Interactive movies"),"Paddle / Pong":qsTranslate("Vetust", "Pong"),Quiz:qsTranslate("Vetust", "Quiz"),"Racing / Driving":qsTranslate("Vetust", "Racing and driving"),"Role-Playing":qsTranslate("Vetust", "Role-playing"),"Text-Based":qsTranslate("Vetust", "Text adventures"),"Vehicle Simulation":qsTranslate("Vetust", "Vehicle simulation")}
        return value === "" ? qsTranslate("Vetust", "All genres") : (names[value] || value)
    }
    function art(id) { return id ? "image://art/" + id + "/" + artRevision : "" }
    function applyFilter() {
        if(unifiedCatalogVisible) return
        if(view === "arcade" || view === "magazines" || view === "scummvm" || view === "atari") return
        vetust.filter(search.text, genre.currentText, view === "home" ? "all" : view, onlyTested, collectionKey)
        catalogGrid.positionViewAtBeginning()
    }
    function navigate(target) {
        catalogDetailsEngine=""; view=target; collectionKey=""; collectionTitle=""; search.clear(); genre.currentIndex=0; onlyTested=false
        debounce.stop(); applyFilter()
        if(target === "home") vetust.refreshHome(true,window.spotlightColumns)
        if(target === "magazines") magazinesView.reset()
        if(target === "scummvm") vetust.scumm.enter()
        if(target === "arcade") vetust.arcade.enter()
        if(target === "atari") vetust.atari.enter()
    }
    function openCollection(row) {
        navigate("all"); collectionKey=row.key; collectionTitle=row.title; applyFilter()
    }
    function openGame(id) { vetust.selectGame(id); details.open() }
    function openCatalogGame(engine, id) {
        if(engine === "dosbox") { openGame(id); return }
        catalogDetailsEngine=engine
        Qt.callLater(function() {
            const loader=engine === "scummvm" ? scummLoader : engine === "atari" ? atariLoader : arcadeLoader
            const source=engine === "scummvm" ? vetust.scumm.games : engine === "atari" ? vetust.atari.games : vetust.arcade.games
            const entry=source.find(g=>(engine === "scummvm" ? g.folder : g.id) === id)
            if(loader.item && entry)loader.item.showGame(entry)
        })
    }
    function fullscreen() { visibility = visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen }
    onClosing: function(close) { if (vetust.busy) { close.accepted=false; busyDialog.open() } }
    Connections {
        target: vetust
        function onStorageChanged() { window.artRevision++ }
        function onHomeChanged() {
            if(window.collectionKey) {
                const row=vetust.homeRows.find(r=>r.key===window.collectionKey)
                if(row)window.collectionTitle=row.title
            }
        }
        function onSelectionChanged() { window.shotIndex=0 }
        function onManualReady(url) { manualDialog.title=qsTranslate("Vetust", "Manual · ") + (game.title || ""); pdfDocument.source=url; manualDialog.open() }
        function onExtraReady(url, name, pdf) {
            if(pdf) { manualDialog.title=name; pdfDocument.source=url; manualDialog.open() }
            else Qt.openUrlExternally(url)
        }
    }
    function sectionBackground(section) {
        const custom=vetust.sectionBackgrounds[section]
        if(custom) return custom
        if(section === "arcade") return Qt.resolvedUrl("assets/arcade-header.png")
        if(section === "scummvm") return Qt.resolvedUrl("assets/scummvm-header.png")
        if(section === "atari") return Qt.resolvedUrl("assets/atari2600-header.png")
        if(section === "magazines") return Qt.resolvedUrl("assets/magazines.jpg")
        return Qt.resolvedUrl("assets/retro-library.jpg")
    }
    function sectionColor(section) {
        return section === "arcade" ? "#323145" : section === "atari" ? "#49382c"
             : section === "magazines" ? "#494133" : "#253b49"
    }
    LibraryBackdrop {
        anchors.fill: parent
        source: window.sectionBackground(window.view)
        baseColor: window.sectionColor(window.view)
    }

    component MenuIcon : Canvas {
        required property string symbol
        property color ink: window.muted
        width: 18; height: 18
        canvasSize: Qt.size(width * Screen.devicePixelRatio, height * Screen.devicePixelRatio)
        onSymbolChanged: requestPaint()
        onInkChanged: requestPaint()
        onCanvasSizeChanged: requestPaint()
        onPaint: {
            const c = getContext("2d")
            c.reset(); c.scale(canvasSize.width / 24, canvasSize.height / 24)
            c.strokeStyle = ink; c.lineWidth = 1.8; c.lineCap = "round"; c.lineJoin = "round"
            function line(points) {
                c.beginPath(); c.moveTo(points[0][0], points[0][1])
                for (let i = 1; i < points.length; ++i) c.lineTo(points[i][0], points[i][1])
                c.stroke()
            }
            function circle(x, y, radius) { c.beginPath(); c.arc(x, y, radius, 0, Math.PI * 2); c.stroke() }
            switch (symbol) {
            case "home":
                line([[3,10],[12,3],[21,10]]); line([[5,9],[5,21],[10,21],[10,14],[14,14],[14,21],[19,21],[19,9]]); break
            case "all":
                for (let x = 4; x <= 14; x += 10) for (let y = 4; y <= 14; y += 10) c.strokeRect(x,y,6,6)
                break
            case "favorites":
                c.beginPath(); c.moveTo(12,21); c.bezierCurveTo(9,18,3,13,3,8); c.bezierCurveTo(3,2,10,2,12,7); c.bezierCurveTo(14,2,21,2,21,8); c.bezierCurveTo(21,13,15,18,12,21); c.stroke(); break
            case "recent":
                circle(12,12,9); line([[12,6],[12,12],[16,14]]); break
            case "scummvm":
                line([[5,3],[5,20],[9,16],[12,21],[16,19],[13,14],[20,14],[5,3]]); break
            case "atari":
                c.strokeRect(3,16,18,5); line([[12,16],[12,8]]); circle(12,5,3); circle(7,18.5,1); break
            case "arcade":
                line([[6,3],[19,3],[18,13],[21,16],[20,21],[4,21],[5,16],[8,13],[6,3]]); c.strokeRect(9,6,7,5); line([[9,16],[16,16]]); break
            case "magazines":
                line([[12,5],[8,3],[2,3],[2,19],[8,19],[12,21],[16,19],[22,19],[22,3],[16,3],[12,5],[12,21]]); line([[5,7],[8,7]]); line([[16,7],[19,7]]); break
            }
        }
    }
    component Action : Button {
        id: action
        property bool primary: false
        property bool highlightedText: false
        implicitHeight: 42
        leftPadding: 16; rightPadding: 16
        contentItem: Text { text: action.text; color: !action.enabled ? "#657583" : action.primary ? "#082323" : action.highlightedText ? window.gold : window.ink; font.pixelSize: 12; font.weight: action.primary ? Font.DemiBold : Font.Normal; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
        background: Rectangle { radius: 5; color: action.primary ? (action.hovered ? "#a0eee7" : window.cyan) : action.hovered ? "#24313e" : "#111c26"; opacity: action.enabled ? 1 : 0.45; border.width: 1; border.color: action.activeFocus ? window.cyan : action.primary ? "#9ce9e2" : "#34404b" }
    }
    component Art : Image { asynchronous: true; fillMode: Image.PreserveAspectFit; sourceSize.width: 800; cache: true }
    component LabelText : Text { color: window.muted; font.pixelSize: 11 }

    component GameTile : Button {
        id: tile
        required property var entry
        implicitWidth: 164; implicitHeight: 244
        padding: 0; hoverEnabled: true
        Accessible.name: entry.title
        ToolTip.text: entry.title; ToolTip.visible: hovered; ToolTip.delay: 800
        onClicked: window.openGame(entry.id)
        Keys.onReturnPressed: clicked()
        Keys.onEnterPressed: clicked()
        background: Item {}
        contentItem: ColumnLayout {
            spacing: 6
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                radius: 7; color: tile.hovered ? "#1c2c39" : "#111c26"
                border.width: tile.activeFocus ? 2 : 1
                border.color: tile.activeFocus ? window.cyan : tile.hovered ? window.gold : "#2b3946"
                Art { anchors.fill: parent; anchors.margins: 5; source: window.art(tile.entry.cover); sourceSize.width: 300 }
                Text { anchors.centerIn: parent; width: parent.width-18; visible: !tile.entry.cover; text: tile.entry.title; color: window.gold; font.pixelSize: 18; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter }
                Rectangle { visible: !!tile.entry.spanishAvailable; anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 8; width: 26; height: 19; radius: 3; color: "#de102a31"; LabelText { anchors.centerIn: parent; text: "ES"; color: window.cyan; font.pixelSize: 9 } }
                Text { visible: !!tile.entry.favorite; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; text: "♥"; color: window.gold; font.pixelSize: 17 }
            }
            Text { Layout.fillWidth: true; Layout.preferredHeight: 30; text: tile.entry.title; color: window.ink; font.pixelSize: 11; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight }
            LabelText { Layout.fillWidth: true; text: (tile.entry.year || "") + " · " + window.genreName((tile.entry.genre || "MS-DOS").split(";")[0]); elide: Text.ElideRight; font.pixelSize: 9 }
        }
    }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: window.width < 1200 ? 20 : 32
        spacing: 16
        RowLayout {
            Layout.fillWidth: true; Layout.preferredHeight: 52; spacing: window.width < 1400 ? 4 : window.width < 1800 ? 8 : 18
            RowLayout {
                spacing: 14
                Rectangle { width: 64; height: 47; radius: 8; color: "#0b1722"; border.color: "#64543f"
                    Text { anchors.centerIn: parent; text: "V:>_"; color: window.gold; font.pixelSize: 23; font.family: "monospace" }
                }
                Column {
                    visible: window.width >= 1480
                    spacing: 5
                    Text { text: "VETUSTware"; color: window.ink; font.pixelSize: window.width < 1400 ? 22 : 27; font.letterSpacing: 1.5; font.weight: Font.DemiBold }
                    Text { text: "Old School Gamer"; color: window.muted; font.pixelSize: 8; font.letterSpacing: 3 }
                }
            }
            Repeater { model: window.sections
                delegate: Button {
                    id: navigationButton
                    required property var modelData
                    objectName: "navigation-"+modelData.value
                    Accessible.name: modelData.label
                    readonly property color navigationColor: window.view===modelData.value ? window.gold : window.muted
                    leftPadding: window.width < 1400 ? 2 : window.width < 1800 ? 4 : 5
                    rightPadding: leftPadding
                    implicitHeight: 48; implicitWidth: contentItem.implicitWidth+leftPadding+rightPadding
                    contentItem: Row {
                        spacing: window.width < 1400 ? 4 : 7
                        MenuIcon { anchors.verticalCenter: parent.verticalCenter; width: window.width < 1400 ? 16 : 18; height: width; symbol: navigationButton.modelData.value; ink: navigationButton.navigationColor }
                        Text { anchors.verticalCenter: parent.verticalCenter; text: navigationButton.modelData.label; color: navigationButton.navigationColor; font.pixelSize: window.width < 1400 ? 10 : 11; font.letterSpacing: window.width < 1800 ? 0.5 : 1.4 }
                    }
                    background: Rectangle { color: "transparent"; Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2; color: window.gold; visible: window.view===modelData.value } }
                    onClicked: window.navigate(modelData.value)
                }
            }
            Item { Layout.fillWidth: true }
            TextField {
                id: search; objectName: "searchField"
                Layout.preferredWidth: window.width < 1400 ? 120 : window.width < 1600 ? 180 : 235
                implicitHeight: 40
                placeholderText: window.view === "magazines" ? qsTranslate("Vetust", "Search magazines…") : qsTranslate("Vetust", "Search games…"); color: window.ink; placeholderTextColor: window.muted; font.pixelSize: 12
                leftPadding: 14; rightPadding: 14; selectByMouse: true
                background: Rectangle { color: "#111c26"; radius: 6; border.color: search.activeFocus ? window.cyan : "#34404b" }
                onTextEdited: {
                    if(window.view === "home") { window.view="all"; window.collectionKey=""; window.collectionTitle=""; genre.currentIndex=0; window.onlyTested=false }
                    debounce.restart()
                }
                Timer { id: debounce; interval: 150; onTriggered: window.applyFilter() }
            }
            Action { text: qsTranslate("Vetust", "Settings"); enabled: !vetust.busy; onClicked: emulatorDialog.begin(true); ToolTip.text: qsTranslate("Vetust", "Global emulator settings"); ToolTip.visible: hovered }
            Action { text: "⛶"; implicitWidth: 40; onClicked: window.fullscreen(); ToolTip.text: qsTranslate("Vetust", "Fullscreen · F11"); ToolTip.visible: hovered }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#25313c" }
        ListView {
            id: homeFeed; objectName: "homeFeed"
            visible: window.view === "home"
            Layout.fillWidth: true; Layout.fillHeight: true
            model: vetust.homeRows; spacing: 16; clip: true; cacheBuffer: 320
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            header: Item {
                width: homeFeed.width; height: window.height < 800 ? 268 : 292
                RowLayout {
                    anchors.fill: parent; anchors.bottomMargin: 20; anchors.rightMargin: 14
                    spacing: 16
                    Repeater {
                        model: vetust.homeSpotlights
                        Rectangle {
                            id: spotlightCard
                            required property var modelData
                            required property int index
                            objectName: "spotlightCard-"+index
                            Layout.fillWidth: true; Layout.fillHeight: true
                            Layout.preferredWidth: 1
                            property bool compact: window.spotlightColumns > 1
                            radius: 10; color: "#122230"; border.color: "#344552"; clip: true
                            Image { anchors.fill: parent; source: window.art(spotlightCard.modelData.background || (spotlightCard.modelData.shots || [])[0]); fillMode: Image.PreserveAspectCrop; opacity: 0.12; asynchronous: true; sourceSize.width: 1280 }
                            RowLayout {
                                anchors.fill: parent; anchors.margins: spotlightCard.compact ? 18 : 22; spacing: spotlightCard.compact ? 18 : 28
                                ColumnLayout {
                                    Layout.fillWidth: true; Layout.preferredWidth: 600; spacing: 8
                                    LabelText { Layout.fillWidth: true; text: spotlightCard.modelData.reason || qsTranslate("Vetust", "DISCOVER THE COLLECTION"); color: window.cyan; elide: Text.ElideRight; font.pixelSize: 11 }
                                    Text { Layout.fillWidth: true; text: spotlightCard.modelData.title || qsTranslate("Vetust", "Your next adventure"); color: "#f1d3a0"; font.family: "Noto Serif"; font.pixelSize: spotlightCard.compact ? 25 : window.width < 1200 ? 27 : 33; wrapMode: Text.WordWrap; maximumLineCount: spotlightCard.compact ? 3 : 2; elide: Text.ElideRight }
                                    LabelText { Layout.fillWidth: true; text: (spotlightCard.modelData.year || "") + " · " + (spotlightCard.modelData.developer || "MS-DOS"); elide: Text.ElideRight }
                                    Text { Layout.fillWidth: true; text: spotlightCard.modelData.summary || qsTranslate("Vetust", "Explore your MS-DOS game collection."); color: "#b6c4cf"; font.pixelSize: 12; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight }
                                    Item { Layout.fillHeight: true }
                                    RowLayout {
                                        Action { text: qsTranslate("Vetust", "Explore this game  →"); primary: true; enabled: !!spotlightCard.modelData.id; onClicked: window.openGame(spotlightCard.modelData.id) }
                                        LabelText { objectName: "catalogTotalLabel"; text: qsTranslate("Vetust", "%1 stories to discover").arg(vetust.catalogCount.toLocaleString(Qt.locale(languages.current),"f",0)); visible: !spotlightCard.compact && window.width >= 1300; font.pixelSize: 10 }
                                    }
                                }
                                Rectangle {
                                    Layout.preferredWidth: parent.width*0.40; Layout.fillHeight: true
                                    radius: 6; color: "#070e14"; border.color: "#3c4b56"
                                    Art { anchors.fill: parent; anchors.margins: 4; source: window.art((spotlightCard.modelData.shots || [])[0] || spotlightCard.modelData.cover); sourceSize.width: 800 }
                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: window.openGame(spotlightCard.modelData.id) }
                                }
                            }
                        }
                    }
                }
            }
            delegate: Item {
                id: section
                required property var modelData
                width: homeFeed.width; height: 306
                RowLayout {
                    id: sectionTitle; anchors.left: parent.left; anchors.right: parent.right; anchors.rightMargin: 16; height: 46; spacing: 12
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 4
                        Text { Layout.fillWidth: true; text: section.modelData.title; color: window.ink; font.pixelSize: 20; elide: Text.ElideRight }
                        LabelText { Layout.fillWidth: true; text: section.modelData.subtitle; elide: Text.ElideRight; font.pixelSize: 10 }
                    }
                    Action { text: qsTranslate("Vetust", "View all · ") + section.modelData.count; implicitHeight: 32; onClicked: window.openCollection(section.modelData) }
                    Action { text: "‹"; implicitWidth: 32; implicitHeight: 32; enabled: !shelf.atXBeginning; onClicked: shelf.contentX=Math.max(0,shelf.contentX-shelf.width*0.8) }
                    Action { text: "›"; implicitWidth: 32; implicitHeight: 32; enabled: !shelf.atXEnd; onClicked: shelf.contentX=Math.min(Math.max(0,shelf.contentWidth-shelf.width),shelf.contentX+shelf.width*0.8) }
                }
                ListView {
                    id: shelf; objectName: "shelf-"+section.modelData.key
                    anchors.top: sectionTitle.bottom; anchors.topMargin: 10; anchors.left: parent.left; anchors.right: parent.right; anchors.rightMargin: 16
                    height: 244; orientation: ListView.Horizontal; spacing: 16
                    model: section.modelData.games; clip: true; cacheBuffer: 180; boundsBehavior: Flickable.StopAtBounds
                    delegate: GameTile {
                        required property var modelData
                        required property int index
                        entry: modelData; width: 164; height: shelf.height
                        onActiveFocusChanged: if(activeFocus)shelf.currentIndex=index
                        Keys.onRightPressed: { shelf.incrementCurrentIndex(); if(shelf.currentItem)shelf.currentItem.forceActiveFocus() }
                        Keys.onLeftPressed: { shelf.decrementCurrentIndex(); if(shelf.currentItem)shelf.currentItem.forceActiveFocus() }
                    }
                }
            }
            footer: Item { width: homeFeed.width; height: 60; LabelText { anchors.centerIn: parent; text: qsTranslate("Vetust", "Your collection, at your pace. All recommendations are made on this computer."); font.pixelSize: 11 } }
        }
        ColumnLayout {
            visible: !window.unifiedCatalogVisible && window.view !== "arcade" && window.view !== "home" && window.view !== "magazines" && window.view !== "scummvm" && window.view !== "atari"
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 16
            RowLayout {
                Layout.fillWidth: true; spacing: 12
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 5
                    LabelText { text: search.text ? qsTranslate("Vetust", "SEARCH RESULTS") : qsTranslate("Vetust", "EXPLORE AT YOUR OWN PACE"); color: window.cyan; font.letterSpacing: 1.5; font.pixelSize: 9 }
                    Text { Layout.fillWidth: true; text: (window.collectionTitle || (window.view==="favorites" ? qsTranslate("Vetust", "Your favorites") : window.view==="recent" ? qsTranslate("Vetust", "Recently played") : qsTranslate("Vetust", "Full catalog"))) + " · " + vetust.count.toLocaleString(Qt.locale(languages.current),"f",0); color: window.ink; font.pixelSize: 21; elide: Text.ElideRight }
                }
                ComboBox { id: genre; objectName: "genreFilter"; model: vetust.genres; displayText: window.genreName(currentText); delegate: ItemDelegate { required property string modelData; width: genre.width; text: window.genreName(modelData); palette.text: window.ink; palette.window: "#121e28" } implicitWidth: window.width < 1200 ? 155 : 190; implicitHeight: 34; font.pixelSize: 10; onActivated: window.applyFilter(); palette.button: "#121e28"; palette.buttonText: window.muted; palette.text: window.ink; palette.base: "#121e28" }
                Action { text: qsTranslate("Vetust", "Verified"); implicitHeight: 34; highlightedText: onlyTested; onClicked: { onlyTested=!onlyTested; window.applyFilter() } ToolTip.text: qsTranslate("Vetust", "Show only games whose startup has been verified"); ToolTip.visible: hovered }
                Action { text: "←"; implicitWidth: 33; implicitHeight: 34; enabled: vetust.page>0; onClicked: { vetust.pageBy(-1); catalogGrid.positionViewAtBeginning() } }
                LabelText { text: (vetust.page+1)+" / "+vetust.pages; font.pixelSize: 10 }
                Action { text: "→"; implicitWidth: 33; implicitHeight: 34; enabled: vetust.page<vetust.pages-1; onClicked: { vetust.pageBy(1); catalogGrid.positionViewAtBeginning() } }
            }
            GridView {
                id: catalogGrid; objectName: "catalogGrid"
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: vetust.pageGames
                cellWidth: Math.floor(width/Math.max(4,Math.floor(width/180))); cellHeight: 274
                boundsBehavior: Flickable.StopAtBounds; cacheBuffer: 280
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                delegate: GameTile {
                    required property var modelData
                    required property int index
                    entry: modelData; width: catalogGrid.cellWidth-16; height: catalogGrid.cellHeight-22
                    onActiveFocusChanged: if(activeFocus)catalogGrid.currentIndex=index
                    Keys.onRightPressed: { catalogGrid.moveCurrentIndexRight(); if(catalogGrid.currentItem)catalogGrid.currentItem.forceActiveFocus() }
                    Keys.onLeftPressed: { catalogGrid.moveCurrentIndexLeft(); if(catalogGrid.currentItem)catalogGrid.currentItem.forceActiveFocus() }
                    Keys.onDownPressed: { catalogGrid.moveCurrentIndexDown(); if(catalogGrid.currentItem)catalogGrid.currentItem.forceActiveFocus() }
                    Keys.onUpPressed: { catalogGrid.moveCurrentIndexUp(); if(catalogGrid.currentItem)catalogGrid.currentItem.forceActiveFocus() }
                }
                Column {
                    visible: vetust.count===0; anchors.centerIn: parent; spacing: 16
                    Text { anchors.horizontalCenter: parent.horizontalCenter; text: window.view === "favorites" && !search.text ? qsTranslate("Vetust", "Mark a heart on a game's details page\nto start your personal selection.") : window.view === "recent" && !search.text ? qsTranslate("Vetust", "Your games will appear here after you finish a session.") : qsTranslate("Vetust", "No games match these filters.\nTry another search."); color: window.muted; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; lineHeight: 1.5 }
                    Action { anchors.horizontalCenter: parent.horizontalCenter; text: qsTranslate("Vetust", "Browse the full catalog"); onClicked: window.navigate("all") }
                }
            }
        }
        Loader {
            id: unifiedLoader; active: window.unifiedCatalogVisible; visible: active
            Layout.fillWidth: true; Layout.fillHeight: true
            sourceComponent: CatalogView { query: search.text; onOpenGame: (engine,id)=>window.openCatalogGame(engine,id) }
        }
        Loader { id: arcadeLoader; active: window.view === "arcade" || window.catalogDetailsEngine === "arcade"; visible: window.view === "arcade"; Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: ArcadeView { query: search.text } }
        Loader { id: atariLoader; active: window.view === "atari" || window.catalogDetailsEngine === "atari"; visible: window.view === "atari"; Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AtariView { query: search.text } }
        Loader { id: scummLoader; active: window.view === "scummvm" || window.catalogDetailsEngine === "scummvm"; visible: window.view === "scummvm"; Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: ScummView { query: search.text } }
        MagazinesView { id: magazinesView; visible: window.view === "magazines"; Layout.fillWidth: true; Layout.fillHeight: true; query: search.text; onNavigationChanged: search.clear() }
        Rectangle { Layout.fillWidth: true; height: 1; color: "#25313c" }
        RowLayout { Layout.fillWidth: true; LabelText { text: window.view === "magazines" ? qsTranslate("Vetust", "Select a collection or magazine     /  Search     F11  Fullscreen") : qsTranslate("Vetust", "Select a cover to open game details     /  Search     F11  Fullscreen"); font.pixelSize: 9 } Item { Layout.fillWidth: true } LabelText { text: qsTranslate("Vetust", "●  LOCAL COLLECTION"); color: "#8ba7b4"; font.pixelSize: 9; font.letterSpacing: 1.2 } }
    }
    Dialog {
        id: details; objectName: "gameDetails"
        background: LibraryBackdrop { source: window.art(game.cover || game.box3d || (shots.length ? shots[0] : "")) }
        parent: Overlay.overlay; x: 0; y: 0; width: parent.width; height: parent.height; margins: 0
        modal: true; focus: true; title: game.title || qsTranslate("Vetust", "Game details")
        standardButtons: Dialog.NoButton
        footer: null
        header: Item {
            implicitHeight: 56
            RowLayout {
                anchors.fill: parent; anchors.margins: 12
                Label { Layout.fillWidth: true; text: details.title; textFormat: Text.PlainText; color: window.gold; font.pixelSize: 14; font.bold: true; elide: Text.ElideRight }
                ToolButton {
                    objectName: "closeGameDetailsButton"; text: "×"; font.pixelSize: 26
                    implicitWidth: 36; implicitHeight: 32
                    Accessible.name: qsTranslate("Vetust", "Close")
                    ToolTip.text: qsTranslate("Vetust", "Close (Esc)"); ToolTip.visible: hovered
                    onClicked: details.close()
                }
            }
        }
        palette.window: "#101c27"; palette.windowText: window.gold; palette.button: "#1b2d3b"; palette.buttonText: window.ink
        contentItem: RowLayout {
            id: hero
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.minimumHeight: 300
            spacing: window.width < 1200 ? 24 : 35
            ColumnLayout {
                Layout.preferredWidth: Math.min(260,window.width*0.18)
                Layout.fillHeight: true; spacing: 18
                Item { Layout.fillHeight: true; Layout.maximumHeight: 12 }
                Art { Layout.fillWidth: true; Layout.fillHeight: true; Layout.maximumHeight: 370; Layout.minimumHeight: 230; source: window.art(game.box3d || game.cover)
                    MouseArea {
                        objectName: "dosDetailPlayImage"
                        anchors.fill: parent; enabled: playDosButton.enabled
                        cursorShape: Qt.PointingHandCursor; hoverEnabled: true
                        ToolTip.text: playDosButton.text; ToolTip.visible: containsMouse && enabled
                        onClicked: playDosButton.clicked()
                    }
                }
                LabelText { Layout.alignment: Qt.AlignHCenter; text: game.realCover ? qsTranslate("Vetust", "FROM THE ORIGINAL COLLECTION") : qsTranslate("Vetust", "GAME SCREENSHOT"); font.pixelSize: 8; font.letterSpacing: 1.6 }
                Item { Layout.fillHeight: true; Layout.maximumHeight: 12 }
            }
            ColumnLayout {
                Layout.fillWidth: true; Layout.preferredWidth: 440; Layout.minimumWidth: 285; spacing: game.hasSpanish ? 7 : window.height < 800 ? 8 : 12
                Row { spacing: 12; Rectangle { width: 5; height: 5; radius: 3; color: window.cyan; anchors.verticalCenter: parent.verticalCenter } LabelText { text: "MS-DOS     —     " + (game.year || ""); font.pixelSize: 10; font.letterSpacing: 2 } }
                Text { Layout.fillWidth: true; text: game.title || "VETUSTware"; color: "#f1d3a0"; font.family: "Noto Serif"; font.pixelSize: window.height < 800 ? 26 : game.hasSpanish ? 32 : game.title.length>42 ? 33 : 39; font.weight: Font.Normal; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight; lineHeight: 1.02 }
                LabelText { Layout.fillWidth: true; text: game.developer || game.publisher || ""; color: "#c2cbd2"; elide: Text.ElideRight; font.pixelSize: 12 }
                Row { spacing: 7
                    Repeater { model: (game.genre || "MS-DOS").split(";").slice(0,3)
                        delegate: Rectangle { required property string modelData; width: tag.implicitWidth+18; height: 25; radius: 4; color: "#141f29"; border.color: "#303e49"; LabelText { id: tag; anchors.centerIn: parent; text: window.genreName(modelData); font.pixelSize: 10 } }
                    }
                }
                Text { Layout.fillWidth: true; text: game.summary || qsTranslate("Vetust", "No description in the catalog."); color: "#a8b6c2"; font.pixelSize: 12; lineHeight: 1.35; wrapMode: Text.WordWrap; maximumLineCount: window.height<800 ? 2 : 3; elide: Text.ElideRight }
                Button { text: qsTranslate("Vetust", "Read full details ↗"); padding: 0; implicitHeight: 22; contentItem: Text { text: parent.text; color: "#c6b08d"; font.pixelSize: 11 } background: Item {} onClicked: infoDialog.open() }
                Flow { spacing: 10; Layout.topMargin: 2; Layout.fillWidth: true
                    Action { id: playDosButton; objectName: "playDos"; text: vetust.busy ? qsTranslate("Vetust", "IN SESSION") : qsTranslate("Vetust", "▶   PLAY"); primary: true; enabled: !!game.playable && !vetust.busy; implicitHeight: 48; implicitWidth: 140; onClicked: vetust.play() }
                    Action { text: game.favorite ? "♥" : "♡"; highlightedText: !!game.favorite; implicitHeight: 48; implicitWidth: 48; onClicked: vetust.toggleFavorite(); ToolTip.text: qsTranslate("Vetust", "Favorite · F"); ToolTip.visible: hovered }
                    Action { text: qsTranslate("Vetust", "▤  Manual"); visible: !!game.manual; implicitHeight: 48; onClicked: vetust.openManual() }
                    Action { objectName: "extrasButton"; text: qsTranslate("Vetust", "Extras"); visible: vetust.gameExtras.length > 0; implicitHeight: 48; onClicked: extrasDialog.begin() }
                }
                RowLayout {
                    visible: !!game.hasSpanish
                    LabelText { text: qsTranslate("Vetust", "EDITION") }
                    ComboBox { id:editionPicker; objectName:"editionPicker"; model:["es","original"]
                        function label(value) { return value==="es" ? qsTranslate("Vetust", "Spanish") : qsTranslate("Vetust", "Original") }
                        displayText:label(currentText)
                        delegate:ItemDelegate { required property string modelData; width:editionPicker.width; text:editionPicker.label(modelData) }
                        currentIndex: game.spanish ? 0 : 1; enabled: !vetust.busy; implicitHeight: 32; implicitWidth: 180; palette.button: "#121e28"; palette.buttonText: window.ink; palette.text: window.ink; palette.base: "#121e28"; onActivated: vetust.setSpanish(currentIndex===0) }
                }
                Flow {
                    Layout.fillWidth: true
                    spacing: 8
                    Action { text: qsTranslate("Vetust", "⚙  Configure this game"); implicitHeight: 32; enabled: !vetust.busy; onClicked: emulatorDialog.begin(false) }
                    Action { objectName: "openDosFolder"; text: qsTranslate("Vetust", "Open folder"); implicitHeight: 32; enabled: !!game.playable && !vetust.busy; onClicked: vetust.openGameFolder() }
                    Action {
                        id: gameTools; objectName: "gameToolsButton"
                        text: "⋯"; implicitHeight: 32; implicitWidth: 44; enabled: !vetust.busy
                        Accessible.name: qsTranslate("Vetust", "Game tools")
                        ToolTip.text: qsTranslate("Vetust", "Game tools"); ToolTip.visible: hovered
                        onClicked: gameToolsMenu.popup()
                        Menu {
                            id: gameToolsMenu; objectName: "gameToolsMenu"
                            y: gameTools.height
                            palette.window: "#172633"; palette.windowText: window.ink; palette.text: window.ink
                            MenuItem { text: qsTranslate("Vetust", "Open DOSBox at C:\\"); enabled: !!game.playable; onTriggered: vetust.openConsole() }
                            MenuItem { text: qsTranslate("Vetust", "Browse with XTree Gold"); enabled: !!game.playable && vetust.xTreeAvailable; onTriggered: vetust.openConsole("xtree") }
                            MenuItem { text: qsTranslate("Vetust", "Browse with Norton Commander"); enabled: !!game.playable && vetust.nortonAvailable; onTriggered: vetust.openConsole("norton") }
                        }
                    }
                }
                LabelText { Layout.fillWidth: true; text: game.profileLabel || ""; wrapMode: Text.WordWrap; font.pixelSize: 10; color: "#8198a9" }
            }
            ColumnLayout {
                Layout.preferredWidth: Math.min(window.width*0.33,500)
                Layout.minimumWidth: 300
                spacing: 14
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: Math.min(hero.height-70,width*0.75)
                    color: "#03070a"; radius: 6; border.color: "#34404b"
                    Art { anchors.fill: parent; anchors.margins: 6; source: window.art(shot); sourceSize.width: 1200 }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: imageDialog.open() }
                    LabelText { anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 12; text: "⛶"; color: "#b9cbd6" }
                }
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    LabelText { text: (shots.length ? String(shotIndex+1).padStart(2,"0") : "00") + " / " + String(shots.length).padStart(2,"0") + qsTranslate("Vetust", " · GALLERY"); font.pixelSize: 8; font.letterSpacing: 1 }
                    Item { Layout.fillWidth: true }
                    Repeater { model: shots.slice(0,4)
                        delegate: Rectangle { required property string modelData; required property int index; width: window.width<1200 ? 40 : 54; height: 40; radius: 3; color: "#0c1822"; border.color: shotIndex===index ? window.gold : "#303e49"; Art { anchors.fill: parent; anchors.margins: 3; source: window.art(modelData); sourceSize.width: 180 } MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: shotIndex=index } }
                    }
                    Button { visible: shots.length>4; text: "→"; implicitWidth: 27; implicitHeight: 40; contentItem: Text { text: "→"; color: window.muted; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter } background: Item {} onClicked: shotIndex=(shotIndex+1)%shots.length }
                }
            }
        }
    }
    Rectangle {
        visible: vetust.status!==""; anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottomMargin: 22
        width: Math.min(window.width-80,statusRow.implicitWidth+40); height: vetust.busy ? 68 : 52; radius: 8; color: "#142a37"; border.color: "#547e87"
        RowLayout { id: statusRow; anchors.centerIn: parent; spacing: 14
            LabelText { text: vetust.status + (vetust.busy && vetust.progress<100 ? " · "+vetust.progress+"%" : ""); color: window.ink; elide: Text.ElideRight; Layout.maximumWidth: window.width-330 }
            Button { text: qsTranslate("Vetust", "Log"); flat: true; implicitHeight: 30; palette.buttonText: window.cyan; onClicked: vetust.openLog() }
            Button { visible: !vetust.busy; text: "×"; flat: true; implicitWidth: 28; implicitHeight: 30; palette.buttonText: window.ink; onClicked: vetust.dismissStatus() }
        }
        Rectangle { visible: vetust.busy && vetust.progress>=0; anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.margins: 3; height: 2; radius: 1; width: (parent.width-6)*vetust.progress/100; color: window.cyan }
    }
    Dialog {
        id: infoDialog; objectName: "gameInfoDialog"; anchors.centerIn: parent; width: Math.min(780,window.width-100); height: Math.min(680,window.height-100); modal: true; focus: true; title: game.title || qsTranslate("Vetust", "Details")
        standardButtons: Dialog.NoButton
        footer: null
        header: Item {
            implicitHeight: 56
            RowLayout {
                anchors.fill: parent; anchors.margins: 12
                Label { Layout.fillWidth: true; text: infoDialog.title; textFormat: Text.PlainText; color: window.gold; font.pixelSize: 14; font.bold: true; elide: Text.ElideRight }
                ToolButton {
                    objectName: "closeGameInfoButton"; text: "×"; font.pixelSize: 26
                    implicitWidth: 36; implicitHeight: 32
                    Accessible.name: qsTranslate("Vetust", "Close")
                    ToolTip.text: qsTranslate("Vetust", "Close (Esc)"); ToolTip.visible: hovered
                    onClicked: infoDialog.close()
                }
            }
        }
        palette.window: "#111e29"; palette.windowText: window.gold; palette.button: "#1b2d3b"; palette.buttonText: window.ink
        ScrollView { anchors.fill: parent; clip: true; TextArea { width: parent.width; readOnly: true; selectByMouse: true; wrapMode: TextEdit.WordWrap; text: (game.year || "")+" · "+(game.developer || "")+"\n\n"+(game.description || qsTranslate("Vetust", "No description in the catalog.")); color: "#bccbd5"; font.pixelSize: 14; background: Item {} } }
    }
    Dialog { id: imageDialog; anchors.centerIn: parent; width: window.width-100; height: window.height-90; modal: true; standardButtons: Dialog.Close; palette.window: "#080f17"; palette.button: "#1b2d3b"; palette.buttonText: window.ink; Art { anchors.fill: parent; source: window.art(shot); sourceSize.width: 1600 } }
    PdfDocument { id: pdfDocument; objectName: "pdfDocument" }
    Dialog {
        id: manualDialog; objectName: "documentDialog"; parent: Overlay.overlay; x: 0; y: 0; width: parent.width; height: parent.height; margins: 0; modal: true; focus: true; title: qsTranslate("Vetust", "Manual · ") + (game.title || "")
        standardButtons: Dialog.NoButton
        footer: null
        palette.window: "#111e29"; palette.windowText: window.ink; palette.button: "#1b2d3b"; palette.buttonText: window.ink
        header: Item {
            implicitHeight: 56
            RowLayout {
                anchors.fill: parent; anchors.margins: 12
                Label { Layout.fillWidth: true; text: manualDialog.title; color: window.ink; font.pixelSize: 14; font.bold: true; elide: Text.ElideRight }
                ToolButton {
                    objectName: "closePdfButton"; text: "×"; font.pixelSize: 26
                    implicitWidth: 36; implicitHeight: 32
                    Accessible.name: qsTranslate("Vetust", "Close PDF viewer")
                    ToolTip.text: qsTranslate("Vetust", "Close (Esc)"); ToolTip.visible: hovered
                    onClicked: manualDialog.close()
                }
            }
        }
        onOpened: Qt.callLater(function() { documentView.fitPage(); documentView.forceActiveFocus() })
        onClosed: pdfDocument.source=""
        Shortcut { sequence: "Left"; enabled: manualDialog.opened && !pdfPageMode.popup.visible; onActivated: documentView.previousPage() }
        Shortcut { sequence: "Right"; enabled: manualDialog.opened && !pdfPageMode.popup.visible; onActivated: documentView.nextPage() }

        contentItem: ColumnLayout {
            spacing: 8
            RowLayout {
                Layout.fillWidth: true
                Action { objectName: "pdfPrevious"; text: qsTranslate("Vetust", "← Previous"); enabled: documentView.canPrevious; onClicked: documentView.previousPage() }
                Label { text: documentView.pageLabel; color: window.ink }
                Action { objectName: "pdfNext"; text: qsTranslate("Vetust", "Next →"); enabled: documentView.canNext; onClicked: documentView.nextPage() }
                ComboBox {
                    id: pdfPageMode; objectName: "pdfPageMode"
                    model: ["single", "double"]
                    function label(mode) { return mode === "double" ? qsTranslate("Vetust", "Two pages") : qsTranslate("Vetust", "Single page") }
                    displayText: label(currentText)
                    delegate: ItemDelegate { required property string modelData; width: pdfPageMode.width; text: pdfPageMode.label(modelData) }
                    currentIndex: documentView.doublePage ? 1 : 0
                    onActivated: documentView.doublePage = currentIndex === 1
                    Accessible.name: qsTranslate("Vetust", "Page layout")
                }
                Item { Layout.fillWidth: true }
                Action { objectName: "pdfFitPage"; text: qsTranslate("Vetust", "Fit page"); onClicked: documentView.fitPage() }
                Action { objectName: "pdfFitWidth"; text: qsTranslate("Vetust", "Fit width"); onClicked: documentView.fitToWidth() }
            }
            PdfReader {
                id: documentView; objectName: "documentView"
                Layout.fillWidth: true; Layout.fillHeight: true; document: pdfDocument
            }
        }
    }
    Dialog { id: busyDialog; anchors.centerIn: parent; width:Math.min(620,window.width-80); modal: true; title: qsTranslate("Vetust", "A task is in progress"); standardButtons: Dialog.Ok; Label { wrapMode:Text.WordWrap; text: qsTranslate("Vetust", "Wait for the current task to finish and close any running emulator before quitting.") } }
    ExtrasDialog { id: extrasDialog }
    BackgroundsDialog {
        id: backgroundsDialog
        sections: window.sections
        previewSource: window.sectionBackground(section)
    }
    RuntimesDialog { id:runtimesDialog }
    StorageDialog { id:storageDialog }
    Component.onCompleted: { vetust.refreshHome(true,window.spotlightColumns); if(vetust.storageSettings.needsSetup)storageDialog.begin() }
    EmulatorDialog { id: emulatorDialog; onBackgroundsRequested: backgroundsDialog.begin(window.view); onStorageRequested:{emulatorDialog.close();storageDialog.begin()} onRuntimesRequested:{emulatorDialog.close();runtimesDialog.begin()} }
    Timer { interval: 60000; running: true; repeat: true; onTriggered: vetust.refreshHome() }
    Shortcut { sequence: "F11"; onActivated: window.fullscreen() }
    Shortcut { sequence: "/"; enabled: !window.modalOpen && !search.activeFocus; onActivated: search.forceActiveFocus() }
    Shortcut { sequence: "F"; enabled: details.opened && !emulatorDialog.visible && !infoDialog.visible && !manualDialog.visible && !imageDialog.visible && !extrasDialog.visible; onActivated: vetust.toggleFavorite() }
}
