import QtQuick
import QtQuick.Controls
import QtQuick.Pdf

Item {
    id: root
    required property PdfDocument document
    property bool doublePage: false
    property int currentPage: 0
    property bool fitWidth: false
    readonly property bool ready: document.source.toString() !== "" && document.status === PdfDocument.Ready && document.pageCount > 0
    // La portada ocupa una vista; los siguientes pares empiezan en índices impares.
    readonly property int firstPage: doublePage && currentPage > 0 ? currentPage - (currentPage % 2 === 0 ? 1 : 0) : currentPage
    readonly property bool hasSecondPage: ready && doublePage && firstPage > 0 && firstPage + 1 < document.pageCount
    readonly property int lastPage: firstPage + (hasSecondPage ? 1 : 0)
    readonly property bool canPrevious: ready && firstPage > 0
    readonly property bool canNext: ready && lastPage + 1 < document.pageCount
    readonly property string pageLabel: (ready ? (firstPage + 1) + (hasSecondPage ? "–" + (lastPage + 1) : "") : "0") + " / " + document.pageCount
    readonly property size firstSize: ready ? document.pagePointSize(firstPage) : Qt.size(1, 1)
    readonly property size secondSize: hasSecondPage ? document.pagePointSize(lastPage) : Qt.size(0, 0)
    readonly property real gap: hasSecondPage ? 8 : 0
    readonly property real spreadScale: Math.max(0.01, fitWidth ? widthScale : Math.min(widthScale, height / Math.max(firstSize.height, secondSize.height)))
    readonly property real widthScale: Math.max(1, width - gap) / (firstSize.width + secondSize.width)

    function showPage(page) {
        if (!ready || page < 0 || page >= document.pageCount) return
        currentPage = page
        fitWidth = false
        refreshView()
    }
    function previousPage() { if (canPrevious) showPage(doublePage ? Math.max(0, firstPage - 2) : currentPage - 1) }
    function nextPage() { if (canNext) showPage(lastPage + 1) }
    function fitPage() { fitWidth = false; refreshView() }
    function fitToWidth() { fitWidth = true; refreshView() }
    function initializeDocument() {
        if (!ready) return
        // Elegir el modo solo al abrir; la navegación respeta la selección manual.
        const cover = document.pagePointSize(0)
        currentPage = 0
        doublePage = document.pageCount > 1 && cover.width > 0 && cover.height > cover.width
        showPage(0)
    }
    function refreshView() {
        if (!ready || width <= 0 || height <= 0) return
        if (!doublePage && singleLoader.item) {
            const singleView = singleLoader.item
            const size = document.pagePointSize(currentPage)
            const scale = fitWidth ? width / size.width : Math.min(width / size.width, height / size.height)
            singleView.goToLocation(currentPage, Qt.point(0, 0), scale)
            singleView.contentX = -singleView.leftMargin
            singleView.contentY = 0
        }
        if (spreadLoader.item) {
            spreadLoader.item.contentX = 0
            spreadLoader.item.contentY = 0
        }
    }
    onDoublePageChanged: Qt.callLater(refreshView)
    onWidthChanged: Qt.callLater(refreshView)
    onHeightChanged: Qt.callLater(refreshView)
    Component.onCompleted: initializeDocument()
    onReadyChanged: {
        if (ready) initializeDocument()
        else currentPage = 0
    }

    Loader {
        id: singleLoader
        anchors.fill: parent
        active: root.ready && !root.doublePage
        onLoaded: { root.refreshView(); item.navigationReady = true }
        sourceComponent: PdfScrollablePageView {
            property bool navigationReady: false
            clip: true
            document: root.document
            leftMargin: Math.max(0, (width - contentWidth) / 2)
            rightMargin: leftMargin
            onLeftMarginChanged: if (contentWidth <= width) contentX = -leftMargin
            onCurrentPageChanged: if (navigationReady && root.ready && !root.doublePage) root.currentPage = currentPage
        }
    }
    Loader {
        id: spreadLoader
        anchors.fill: parent
        active: root.ready
        visible: root.doublePage
        sourceComponent: Flickable {
            id: spreadView
            objectName: "pdfSpreadView"
            anchors.fill: parent
            visible: root.doublePage && root.ready
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            contentWidth: width
            contentHeight: Math.max(height, spread.height)
            ScrollBar.vertical: ScrollBar { }
            Row {
                id: spread
                objectName: "pdfSpread"
                x: Math.max(0, (spreadView.width - width) / 2)
                spacing: root.gap
                Rectangle {
                    color: "white"
                    width: root.firstSize.width * root.spreadScale
                    height: root.firstSize.height * root.spreadScale
                    PdfPageImage {
                        objectName: "pdfLeftPage"
                        anchors.fill: parent
                        document: root.document
                        currentFrame: root.firstPage
                        asynchronous: true
                        sourceSize.width: Math.ceil(width * Screen.devicePixelRatio)
                        sourceSize.height: 0
                    }
                }
                Rectangle {
                    visible: root.hasSecondPage
                    color: "white"
                    width: root.secondSize.width * root.spreadScale
                    height: root.secondSize.height * root.spreadScale
                    PdfPageImage {
                        objectName: "pdfRightPage"
                        anchors.fill: parent
                        document: root.document
                        currentFrame: root.hasSecondPage ? root.lastPage : 0
                        asynchronous: true
                        sourceSize.width: Math.max(1, Math.ceil(width * Screen.devicePixelRatio))
                        sourceSize.height: 0
                    }
                }
            }
        }
    }
}
