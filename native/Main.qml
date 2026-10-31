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
    title: "VETUSTware · The DOS Collection"
    color: "#080f17"
    property color gold: "#e9bd79"
    property color cyan: "#78d8d1"
    property color ink: "#e7edf1"
    property color muted: "#91a3b3"
    property var game: vetust.selected
    property string view: "home"
    property string collectionKey: ""
    property string collectionTitle: ""
    property var spotlight: vetust.homeSpotlight
    property int artRevision: 0
    property bool modalOpen: storageDialog.visible || details.visible || emulatorDialog.visible || infoDialog.visible || imageDialog.visible || manualDialog.visible || extrasDialog.visible || busyDialog.visible
    property bool onlyTested: false
    property int shotIndex: 0
    property var shots: game.shots || []
    property string shot: shots.length ? shots[Math.min(shotIndex,shots.length-1)] : (game.cover || "")
    function genreName(value) {
        const names = {Adventure:"Aventura",Action:"Acción",Platform:"Plataformas",Puzzle:"Puzles",Strategy:"Estrategia",Sports:"Deportes",Simulation:"Simulación",Shooter:"Disparos",Education:"Educativo","Board / Party Game":"Mesa y fiesta","Construction and Management Simulation":"Construcción y gestión",Fighting:"Lucha","First Person Shooter":"Disparos en primera persona","Flight Simulator":"Simulación de vuelo","Game Show":"Concurso","Interactive Fiction":"Ficción interactiva","Interactive Movie":"Película interactiva","Paddle / Pong":"Pong",Quiz:"Preguntas","Racing / Driving":"Conducción","Role-Playing":"Rol","Text-Based":"Aventura de texto","Vehicle Simulation":"Simulación de vehículos"}
        return names[value] || value
    }
    function art(id) { return id ? "image://art/" + id + "/" + artRevision : "" }
    function applyFilter() {
        if(view === "magazines") return
        vetust.filter(search.text, genre.currentText, view === "home" ? "all" : view, onlyTested, collectionKey)
        catalogGrid.positionViewAtBeginning()
    }
    function navigate(target) {
        view=target; collectionKey=""; collectionTitle=""; search.clear(); genre.currentIndex=0; onlyTested=false
        debounce.stop(); applyFilter()
        if(target === "home") vetust.refreshHome()
        if(target === "magazines") magazinesView.reset()
    }
    function openCollection(row) {
        navigate("all"); collectionKey=row.key; collectionTitle=row.title; applyFilter()
    }
    function openGame(id) { vetust.selectGame(id); details.open() }
    function fullscreen() { visibility = visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen }
    onClosing: function(close) { if (vetust.busy) { close.accepted=false; busyDialog.open() } }
    Connections {
        target: vetust
        function onStorageChanged() { window.artRevision++ }
        function onSelectionChanged() { window.shotIndex=0 }
        function onManualReady(url) { manualDialog.title="Manual · " + (game.title || ""); pdfDocument.source=url; manualDialog.open() }
        function onExtraReady(url, name, pdf) {
            if(pdf) { manualDialog.title=name; pdfDocument.source=url; manualDialog.open() }
            else Qt.openUrlExternally(url)
        }
    }
    Image { anchors.fill: parent; source: window.art(view === "home" ? (spotlight.background || spotlight.cover) : (game.background || shot)); fillMode: Image.PreserveAspectCrop; opacity: 0.07; asynchronous: true; sourceSize.width: 1400 }
    Rectangle { anchors.fill: parent; gradient: Gradient { GradientStop { position: 0; color: "#50081018" } GradientStop { position: 1; color: "#f0080f17" } } }

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
            Layout.fillWidth: true; Layout.preferredHeight: 52; spacing: window.width < 1200 ? 12 : 24
            RowLayout {
                spacing: 14
                Rectangle { width: 64; height: 47; radius: 8; color: "#0b1722"; border.color: "#64543f"
                    Text { anchors.centerIn: parent; text: "V:>_"; color: window.gold; font.pixelSize: 23; font.family: "monospace" }
                }
                Column {
                    spacing: 5
                    Text { text: "VETUSTware"; color: window.ink; font.pixelSize: window.width < 1200 ? 22 : 27; font.letterSpacing: 1.5; font.weight: Font.DemiBold }
                    Text { text: "THE DOS COLLECTION"; color: window.muted; font.pixelSize: 8; font.letterSpacing: 3 }
                }
            }
            Repeater { model: [{label:"INICIO",value:"home"},{label:"CATÁLOGO",value:"all"},{label:"FAVORITOS",value:"favorites"},{label:"RECIENTES",value:"recent"},{label:"REVISTAS",value:"magazines"}]
                delegate: Button {
                    required property var modelData
                    implicitHeight: 48; implicitWidth: contentItem.implicitWidth+10
                    contentItem: Text { text: modelData.label; color: window.view===modelData.value ? window.gold : window.muted; font.pixelSize: 11; font.letterSpacing: 1.4; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { color: "transparent"; Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2; color: window.gold; visible: window.view===modelData.value } }
                    onClicked: window.navigate(modelData.value)
                }
            }
            Item { Layout.fillWidth: true }
            TextField {
                id: search; objectName: "searchField"
                Layout.preferredWidth: window.width < 1200 ? 155 : 235
                implicitHeight: 40
                placeholderText: window.view === "magazines" ? "Buscar revistas…" : "Buscar juegos…"; color: window.ink; placeholderTextColor: window.muted; font.pixelSize: 12
                leftPadding: 14; rightPadding: 14; selectByMouse: true
                background: Rectangle { color: "#111c26"; radius: 6; border.color: search.activeFocus ? window.cyan : "#34404b" }
                onTextEdited: {
                    if(window.view === "home") { window.view="all"; window.collectionKey=""; window.collectionTitle=""; genre.currentIndex=0; window.onlyTested=false }
                    debounce.restart()
                }
                Timer { id: debounce; interval: 150; onTriggered: window.applyFilter() }
            }
            Action { text: "Ajustes"; enabled: !vetust.busy; onClicked: emulatorDialog.begin(true); ToolTip.text: "Configuración global de emuladores"; ToolTip.visible: hovered }
            Action { text: "⛶"; implicitWidth: 40; onClicked: window.fullscreen(); ToolTip.text: "Pantalla completa · F11"; ToolTip.visible: hovered }
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
                Rectangle {
                    anchors.fill: parent; anchors.bottomMargin: 20; anchors.rightMargin: 14
                    radius: 10; color: "#122230"; border.color: "#344552"; clip: true
                    Image { anchors.fill: parent; source: window.art(window.spotlight.background || (window.spotlight.shots || [])[0]); fillMode: Image.PreserveAspectCrop; opacity: 0.12; asynchronous: true; sourceSize.width: 1280 }
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 22; spacing: 28
                        ColumnLayout {
                            Layout.fillWidth: true; Layout.preferredWidth: 600; spacing: 8
                            LabelText { Layout.fillWidth: true; text: window.spotlight.reason || "DESCUBRE LA COLECCIÓN"; color: window.cyan; elide: Text.ElideRight; font.pixelSize: 11 }
                            Text { Layout.fillWidth: true; text: window.spotlight.title || "Tu próxima aventura"; color: "#f1d3a0"; font.family: "Noto Serif"; font.pixelSize: window.width < 1200 ? 27 : 33; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight }
                            LabelText { Layout.fillWidth: true; text: (window.spotlight.year || "") + " · " + (window.spotlight.developer || "MS-DOS"); elide: Text.ElideRight }
                            Text { Layout.fillWidth: true; text: window.spotlight.summary || "Explora tu archivo de juegos MS-DOS."; color: "#b6c4cf"; font.pixelSize: 12; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight }
                            Item { Layout.fillHeight: true }
                            RowLayout {
                                Action { text: "Explorar este juego  →"; primary: true; enabled: !!window.spotlight.id; onClicked: window.openGame(window.spotlight.id) }
                                LabelText { text: "7.200 historias por descubrir"; visible: window.width >= 1300; font.pixelSize: 10 }
                            }
                        }
                        Rectangle {
                            Layout.preferredWidth: parent.width*0.40; Layout.fillHeight: true
                            radius: 6; color: "#070e14"; border.color: "#3c4b56"
                            Art { anchors.fill: parent; anchors.margins: 4; source: window.art((window.spotlight.shots || [])[0] || window.spotlight.cover); sourceSize.width: 800 }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: window.openGame(window.spotlight.id) }
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
                    Action { text: "Ver todo · " + section.modelData.count; implicitHeight: 32; onClicked: window.openCollection(section.modelData) }
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
            footer: Item { width: homeFeed.width; height: 60; LabelText { anchors.centerIn: parent; text: "Tu colección, a tu ritmo. Todo se recomienda en este equipo."; font.pixelSize: 11 } }
        }
        ColumnLayout {
            visible: window.view !== "home" && window.view !== "magazines"
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 16
            RowLayout {
                Layout.fillWidth: true; spacing: 12
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 5
                    LabelText { text: search.text ? "RESULTADOS DE BÚSQUEDA" : "EXPLORA A TU RITMO"; color: window.cyan; font.letterSpacing: 1.5; font.pixelSize: 9 }
                    Text { Layout.fillWidth: true; text: (window.collectionTitle || (window.view==="favorites" ? "Tus favoritos" : window.view==="recent" ? "Jugados recientemente" : "Todo el catálogo")) + " · " + vetust.count.toLocaleString(Qt.locale("es_UY"),"f",0); color: window.ink; font.pixelSize: 21; elide: Text.ElideRight }
                }
                ComboBox { id: genre; objectName: "genreFilter"; model: vetust.genres; displayText: window.genreName(currentText); delegate: ItemDelegate { required property string modelData; width: genre.width; text: window.genreName(modelData); palette.text: window.ink; palette.window: "#121e28" } implicitWidth: window.width < 1200 ? 155 : 190; implicitHeight: 34; font.pixelSize: 10; onActivated: window.applyFilter(); palette.button: "#121e28"; palette.buttonText: window.muted; palette.text: window.ink; palette.base: "#121e28" }
                Action { text: "Verificados"; implicitHeight: 34; highlightedText: onlyTested; onClicked: { onlyTested=!onlyTested; window.applyFilter() } ToolTip.text: "Mostrar solo juegos con arranque comprobado"; ToolTip.visible: hovered }
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
                    Text { anchors.horizontalCenter: parent.horizontalCenter; text: window.view === "favorites" && !search.text ? "Marca un corazón en la ficha de un juego\npara empezar tu selección personal." : window.view === "recent" && !search.text ? "Aquí aparecerán tus juegos cuando termines una sesión." : "No hay juegos con estos filtros.\nPrueba otra búsqueda."; color: window.muted; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; lineHeight: 1.5 }
                    Action { anchors.horizontalCenter: parent.horizontalCenter; text: "Ver todo el catálogo"; onClicked: window.navigate("all") }
                }
            }
        }
        MagazinesView { id: magazinesView; visible: window.view === "magazines"; Layout.fillWidth: true; Layout.fillHeight: true; query: search.text; onNavigationChanged: search.clear() }
        Rectangle { Layout.fillWidth: true; height: 1; color: "#25313c" }
        RowLayout { Layout.fillWidth: true; LabelText { text: window.view === "magazines" ? "Selecciona una colección o revista     /  Buscar     F11  Pantalla completa" : "Selecciona una carátula para abrir su ficha     /  Buscar     F11  Pantalla completa"; font.pixelSize: 9 } Item { Layout.fillWidth: true } LabelText { text: "●  COLECCIÓN LOCAL"; color: "#8ba7b4"; font.pixelSize: 9; font.letterSpacing: 1.2 } }
    }
    Dialog {
        id: details; objectName: "gameDetails"
        anchors.centerIn: parent; width: Math.min(1320,window.width-60); height: Math.min(640,window.height-50)
        modal: true; focus: true; title: game.title || "Ficha del juego"
        footer: Item { height: 56; Action { text: "Cerrar"; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 12; implicitWidth: 100; onClicked: details.close() } }
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
                Art { Layout.fillWidth: true; Layout.fillHeight: true; Layout.maximumHeight: 370; Layout.minimumHeight: 230; source: window.art(game.box3d || game.cover) }
                LabelText { Layout.alignment: Qt.AlignHCenter; text: game.realCover ? "DEL ARCHIVO ORIGINAL" : "PANTALLA DEL JUEGO"; font.pixelSize: 8; font.letterSpacing: 1.6 }
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
                Text { Layout.fillWidth: true; text: game.summary || "Sin descripción en el catálogo."; color: "#a8b6c2"; font.pixelSize: 12; lineHeight: 1.35; wrapMode: Text.WordWrap; maximumLineCount: window.height<800 ? 2 : 3; elide: Text.ElideRight }
                Button { text: "Leer ficha completa ↗"; padding: 0; implicitHeight: 22; contentItem: Text { text: parent.text; color: "#c6b08d"; font.pixelSize: 11 } background: Item {} onClicked: infoDialog.open() }
                Flow { spacing: 10; Layout.topMargin: 2; Layout.fillWidth: true
                    Action { text: vetust.busy ? "EN SESIÓN" : "▶   JUGAR"; primary: true; enabled: !!game.playable && !vetust.busy; implicitHeight: 48; implicitWidth: 140; onClicked: vetust.play() }
                    Action { text: game.favorite ? "♥" : "♡"; highlightedText: !!game.favorite; implicitHeight: 48; implicitWidth: 48; onClicked: vetust.toggleFavorite(); ToolTip.text: "Favorito · F"; ToolTip.visible: hovered }
                    Action { text: "▤  Manual"; visible: !!game.manual; implicitHeight: 48; onClicked: vetust.openManual() }
                    Action { objectName: "extrasButton"; text: "Extras"; visible: vetust.gameExtras.length > 0; implicitHeight: 48; onClicked: extrasDialog.begin() }
                }
                RowLayout {
                    visible: !!game.hasSpanish
                    LabelText { text: "VERSIÓN" }
                    ComboBox { model: ["Español", "Original"]; currentIndex: game.spanish ? 0 : 1; enabled: !vetust.busy; implicitHeight: 32; implicitWidth: 180; palette.button: "#121e28"; palette.buttonText: window.ink; palette.text: window.ink; palette.base: "#121e28"; onActivated: vetust.setSpanish(currentIndex===0) }
                }
                RowLayout {
                    spacing: 8
                    Action { text: "⚙  Configurar este juego"; implicitHeight: 32; enabled: !vetust.busy; onClicked: emulatorDialog.begin(false) }
                    Action {
                        id: gameTools; objectName: "gameToolsButton"
                        text: "⋯"; implicitHeight: 32; implicitWidth: 44; enabled: !vetust.busy
                        Accessible.name: "Herramientas del juego"
                        ToolTip.text: "Herramientas del juego"; ToolTip.visible: hovered
                        onClicked: gameToolsMenu.popup()
                        Menu {
                            id: gameToolsMenu; objectName: "gameToolsMenu"
                            y: gameTools.height
                            palette.window: "#172633"; palette.windowText: window.ink; palette.text: window.ink
                            MenuItem { text: "Abrir DOSBox en C:\\"; enabled: !!game.playable; onTriggered: vetust.openConsole() }
                            MenuItem { text: "Explorar con XTree Gold"; enabled: !!game.playable && vetust.xTreeAvailable; onTriggered: vetust.openConsole("xtree") }
                            MenuItem { text: "Explorar con Norton Commander"; enabled: !!game.playable && vetust.nortonAvailable; onTriggered: vetust.openConsole("norton") }
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
                    LabelText { text: (shots.length ? String(shotIndex+1).padStart(2,"0") : "00") + " / " + String(shots.length).padStart(2,"0") + " · GALERÍA"; font.pixelSize: 8; font.letterSpacing: 1 }
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
            Button { text: "Registro"; flat: true; implicitHeight: 30; palette.buttonText: window.cyan; onClicked: vetust.openLog() }
            Button { visible: !vetust.busy; text: "×"; flat: true; implicitWidth: 28; implicitHeight: 30; palette.buttonText: window.ink; onClicked: vetust.dismissStatus() }
        }
        Rectangle { visible: vetust.busy && vetust.progress>=0; anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.margins: 3; height: 2; radius: 1; width: (parent.width-6)*vetust.progress/100; color: window.cyan }
    }
    Dialog {
        id: infoDialog; anchors.centerIn: parent; width: Math.min(780,window.width-100); height: Math.min(680,window.height-100); modal: true; title: game.title || "Ficha"; standardButtons: Dialog.Close
        palette.window: "#111e29"; palette.windowText: window.gold; palette.button: "#1b2d3b"; palette.buttonText: window.ink
        ScrollView { anchors.fill: parent; clip: true; TextArea { width: parent.width; readOnly: true; selectByMouse: true; wrapMode: TextEdit.WordWrap; text: (game.year || "")+" · "+(game.developer || "")+"\n\n"+(game.description || "Sin descripción en el catálogo."); color: "#bccbd5"; font.pixelSize: 14; background: Item {} } }
    }
    Dialog { id: imageDialog; anchors.centerIn: parent; width: window.width-100; height: window.height-90; modal: true; standardButtons: Dialog.Close; palette.window: "#080f17"; palette.button: "#1b2d3b"; palette.buttonText: window.ink; Art { anchors.fill: parent; source: window.art(shot); sourceSize.width: 1600 } }
    PdfDocument { id: pdfDocument; objectName: "pdfDocument"; onStatusChanged: if(status === PdfDocument.Ready) Qt.callLater(function(){documentView.showPage(0)}) }
    Dialog {
        id: manualDialog; objectName: "documentDialog"; anchors.centerIn: parent; width: window.width-100; height: window.height-80; modal: true; focus: true; title: "Manual · " + (game.title || "")
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
                    Accessible.name: "Cerrar visor PDF"
                    ToolTip.text: "Cerrar (Esc)"; ToolTip.visible: hovered
                    onClicked: manualDialog.close()
                }
            }
        }
        onOpened: Qt.callLater(documentView.fitPage)
        onClosed: pdfDocument.source=""
        contentItem: ColumnLayout {
            spacing: 8
            RowLayout {
                Layout.fillWidth: true
                Action { text: "← Anterior"; enabled: documentView.currentPage > 0; onClicked: documentView.showPage(documentView.currentPage - 1) }
                Label { text: (pdfDocument.pageCount ? documentView.currentPage + 1 : 0) + " / " + pdfDocument.pageCount; color: window.ink }
                Action { text: "Siguiente →"; enabled: documentView.currentPage + 1 < pdfDocument.pageCount; onClicked: documentView.showPage(documentView.currentPage + 1) }
                Item { Layout.fillWidth: true }
                Action { text: "Página completa"; onClicked: documentView.fitPage() }
                Action { text: "Ajustar al ancho"; onClicked: documentView.scaleToWidth(documentView.width, documentView.height) }
            }
            PdfScrollablePageView {
                id: documentView; objectName: "documentView"; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; document: pdfDocument
                leftMargin: Math.max(0,(width-contentWidth)/2)
                rightMargin: leftMargin
                onLeftMarginChanged: if(contentWidth <= width) contentX=-leftMargin
                function showPage(page) {
                    if(page < 0 || page >= pdfDocument.pageCount || width <= 0 || height <= 0) return
                    const size = pdfDocument.pagePointSize(page)
                    goToLocation(page, Qt.point(0, 0), Math.min(width / size.width, height / size.height))
                    contentX = -leftMargin; contentY = 0
                }
                function fitPage() { showPage(Math.max(0, currentPage)) }
            }
        }
    }
    Dialog { id: busyDialog; anchors.centerIn: parent; modal: true; title: "Hay una tarea en curso"; standardButtons: Dialog.Ok; Label { text: "Espera a que termine la preparación del archivo o del juego.\nSi DOSBox está abierto, ciérralo con Ctrl+F9 antes de salir." } }
    ExtrasDialog { id: extrasDialog }
    StorageDialog { id:storageDialog }
    Component.onCompleted: { if(vetust.storageSettings.needsSetup)storageDialog.begin() }
    EmulatorDialog { id: emulatorDialog; onStorageRequested:{emulatorDialog.close();storageDialog.begin()} }
    Timer { interval: 60000; running: true; repeat: true; onTriggered: vetust.refreshHome() }
    Shortcut { sequence: "F11"; onActivated: window.fullscreen() }
    Shortcut { sequence: "/"; enabled: !window.modalOpen && !search.activeFocus; onActivated: search.forceActiveFocus() }
    Shortcut { sequence: "F"; enabled: details.opened && !emulatorDialog.visible && !infoDialog.visible && !manualDialog.visible && !imageDialog.visible && !extrasDialog.visible; onActivated: vetust.toggleFavorite() }
}
