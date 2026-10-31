import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog
    signal storageRequested()
    objectName: "emulatorDialog"
    anchors.centerIn: parent
    width: Math.min(880,parent.width-50)
    height: Math.min(advancedExpanded ? 860 : (globalScope ? 630 : 520),parent.height-50)
    modal: true
    closePolicy: Popup.CloseOnEscape
    palette.window: "#111e29"; palette.windowText: "#e7edf1"
    palette.text: "#e7edf1"; palette.base: "#0b151f"
    palette.button: "#233443"; palette.buttonText: "#e7edf1"
    palette.highlight: "#377f87"; palette.highlightedText: "white"
    title: globalScope ? qsTranslate("Vetust", "Global emulation settings") : qsTranslate("Vetust", "Emulation · ") + vetust.selected.title
    Connections {
        target: languages
        function onChanged() {
            if(dialog.opened) {
                const engines=vetust.emulatorSettings(dialog.globalScope).engines
                dialog.engineChoices=dialog.globalScope ? engines : [{id:"",name:qsTranslate("Vetust", "Use global engine"),available:true}].concat(engines)
                dialog.refresh()
            }
        }
    }
    property bool globalScope: false
    property bool advancedExpanded: false
    property var snapshot: ({})
    property var draft: ({})
    property var paths: ({})
    property var engineChoices: []
    property string editEngine: "staging"
    property var inherited: ({})
    property int revision: 0
    property string error: ""
    property var monitorChoices: []
    function refreshMonitorChoices() {
        let choices=[{id:"",label:qsTranslate("Vetust", "Automatic (chosen by the emulator)")}].concat(vetust.monitors)
        let saved=draft.monitor || ""
        if(saved!=="" && !choices.some(m=>m.id===saved)) choices.push({id:saved,label:saved+qsTranslate("Vetust", " · Disconnected")})
        monitorChoices=choices
        monitorPicker.currentIndex=Math.max(0,choices.findIndex(m=>m.id===saved))
    }
    Connections { target:vetust; function onMonitorsChanged(){dialog.refreshMonitorChoices()} }
    Timer { interval:2000; running:dialog.opened; repeat:true; onTriggered:vetust.refreshMonitors() }
    function begin(global) {
        globalScope=global
        advancedExpanded=false
        snapshot=vetust.emulatorSettings(global)
        draft=JSON.parse(JSON.stringify(snapshot.scope))
        if(!draft.options) draft.options={}
        paths=JSON.parse(JSON.stringify(snapshot.paths))
        engineChoices=global ? snapshot.engines : [{id:"",name:qsTranslate("Vetust", "Use global engine"),available:true}].concat(snapshot.engines)
        error=""
        vetust.refreshMonitors()
        refresh()
        open()
    }
    function refresh() {
        let chosen=draft.engine || ""
        enginePicker.currentIndex=Math.max(0,engineChoices.findIndex(e=>e.id===chosen))
        editEngine=chosen || snapshot.globalEngine
        inherited=vetust.inheritedSettings(globalScope,editEngine)
        if(globalScope && !(draft.options[editEngine] || {})["sdl.fullscreen"])setValue("sdl.fullscreen","true")
        fields.model=vetust.emulatorFields(editEngine).filter(f=>f.key!=="sdl.fullscreen" && f.key!=="render.shader" && !(editEngine==="staging" && f.key==="render.aspect"))
        pathField.text=paths[editEngine] || ""
        refreshMonitorChoices()
        revision++
    }
    function value(key) { let r=revision; return ((draft.options || {})[editEngine] || {})[key] || "" }
    function setValue(key,value) {
        if(!draft.options[editEngine])draft.options[editEngine]={}
        if(value.trim()==="")delete draft.options[editEngine][key]
        else draft.options[editEngine][key]=value.trim()
    }
    function availableText() {
        let e=(snapshot.engines || []).find(e=>e.id===editEngine)
        return e && !e.available ? qsTranslate("Vetust", "This emulator is not available. ") + (dialog.globalScope ? qsTranslate("Vetust", "Set its executable in Advanced options.") : qsTranslate("Vetust", "Check Global settings.")) : ""
    }
    contentItem: ColumnLayout {
        spacing: 12
        ColumnLayout {
            visible:dialog.globalScope
            Layout.fillWidth:true; spacing:4
            RowLayout {
                Layout.fillWidth:true
                Label { text:qsTranslate("Vetust", "Interface language"); Layout.preferredWidth:185 }
                ComboBox {
                    id:languagePicker; objectName:"languagePicker"; Layout.fillWidth:true
                    model:languages.available; textRole:"name"
                    currentIndex:Math.max(0,languages.available.findIndex(l=>l.code===languages.current))
                    Accessible.name:qsTranslate("Vetust", "Interface language")
                    onActivated:dialog.error=languages.setLanguage(languages.available[currentIndex].code)
                }
            }
            Label {
                Layout.fillWidth:true; Layout.leftMargin:195; wrapMode:Text.WordWrap
                color:"#91a3b3"; font.pixelSize:11
                text:qsTranslate("Vetust", "Applied and saved immediately. Game languages are unchanged.")
            }
        }
        Label { Layout.fillWidth:true; wrapMode:Text.WordWrap; text: dialog.globalScope ? qsTranslate("Vetust", "Choose how your games open. Each engine keeps its own options.") : qsTranslate("Vetust", "Customize how this game opens. Advanced options let you adjust its emulation."); color:"#a8bac8" }
        ColumnLayout {
            objectName:"monitorSection"
            visible:dialog.globalScope && vetust.monitors.length>1
            Layout.fillWidth:true; spacing:4
            RowLayout {
                Layout.fillWidth:true
                Label { text:qsTranslate("Vetust", "Game monitor"); Layout.preferredWidth:185 }
                ComboBox {
                    id:monitorPicker; objectName:"monitorPicker"; Layout.fillWidth:true
                    model:dialog.monitorChoices; textRole:"label"
                    onActivated:dialog.draft.monitor=dialog.monitorChoices[currentIndex].id
                }
            }
            Label {
                Layout.leftMargin:195; Layout.fillWidth:true; wrapMode:Text.WordWrap; font.pixelSize:11; color:"#91a3b3"
                text:qsTranslate("Vetust", "Applies to games and the DOS console. If the monitor is disconnected, an available one is used.")
            }
        }
        RowLayout {
            Label { text: qsTranslate("Vetust", "Engine"); Layout.preferredWidth: 185 }
            ComboBox {
                id: enginePicker; Layout.fillWidth:true; model: dialog.engineChoices; textRole:"name"
                onActivated: {
                    dialog.draft.engine=dialog.engineChoices[currentIndex].id
                    dialog.refresh()
                }
            }
        }
        Label { visible:text!==""; Layout.fillWidth:true; wrapMode:Text.WordWrap; text: dialog.availableText(); color:"#ffb1a4"; font.pixelSize:11 }
        RowLayout {
            Layout.fillWidth:true
            Label { text:qsTranslate("Vetust", "Fullscreen"); Layout.preferredWidth:185 }
            Switch {
                objectName:"fullscreenSwitch"
                checked:dialog.value("sdl.fullscreen")!=="" ? dialog.value("sdl.fullscreen")==="true" : (dialog.inherited["sdl.fullscreen"] || "").startsWith("true")
                text:checked ? qsTranslate("Vetust", "On") : qsTranslate("Vetust", "Off")
                Accessible.name:qsTranslate("Vetust", "Fullscreen")
                onToggled:dialog.setValue("sdl.fullscreen",checked ? "true" : "false")
            }
            Item { Layout.fillWidth:true }
        }
        ColumnLayout {
            visible:dialog.editEngine==="staging"
            Layout.fillWidth:true; spacing:4
            RowLayout {
                Layout.fillWidth:true
                Label { text:qsTranslate("Vetust", "Image style"); Layout.preferredWidth:185 }
                ComboBox {
                    id:imageStyle; objectName:"imageStylePicker"; Layout.fillWidth:true
                    property string globalLabel:dialog.inherited["render.shader"]==="crt-auto" ? qsTranslate("Vetust", "Classic monitor (CRT)") : qsTranslate("Vetust", "Sharp")
                    model:dialog.globalScope ? ["crt-auto","sharp"] : ["","crt-auto","sharp"]
                    function label(value) { return value==="" ? qsTranslate("Vetust", "Use global setting · ")+globalLabel : value==="sharp" ? qsTranslate("Vetust", "Sharp") : qsTranslate("Vetust", "Classic monitor (CRT)") }
                    displayText:label(currentText)
                    delegate:ItemDelegate { required property string modelData; width:imageStyle.width; text:imageStyle.label(modelData) }
                    currentIndex:dialog.globalScope ? (dialog.value("render.shader")==="crt-auto" ? 0 : 1) : (dialog.value("render.shader")==="" ? 0 : dialog.value("render.shader")==="sharp" ? 2 : 1)
                    Accessible.name:qsTranslate("Vetust", "Image style")
                    onActivated:{
                        dialog.setValue("render.shader",!dialog.globalScope && currentIndex===0 ? "" : currentIndex===(dialog.globalScope ? 1 : 2) ? "sharp" : "crt-auto")
                        dialog.revision++
                    }
                }
            }
            Label {
                Layout.leftMargin:195; Layout.fillWidth:true; wrapMode:Text.WordWrap; font.pixelSize:11; color:"#91a3b3"
                text:qsTranslate("Vetust", "Both preserve the aspect ratio. Sharp uses more of the screen; CRT recreates the scanlines of a classic monitor.")
            }
        }
        ToolButton {
            objectName:"advancedToggle"
            text:(dialog.advancedExpanded ? "▾ " : "▸ ")+qsTranslate("Vetust", "Advanced options")
            onClicked:dialog.advancedExpanded=!dialog.advancedExpanded
            Accessible.name:qsTranslate("Vetust", "Advanced options")
            Accessible.description:dialog.advancedExpanded ? qsTranslate("Vetust", "Expanded") : qsTranslate("Vetust", "Collapsed")
        }
        Button { visible:dialog.globalScope; text:qsTranslate("Vetust", "Library and folders…"); onClicked:dialog.storageRequested() }
        Item { visible:!dialog.advancedExpanded; Layout.fillHeight:true }
        ScrollView {
            visible:dialog.advancedExpanded
            Layout.fillWidth:true; Layout.fillHeight:true; clip:true
            contentWidth:availableWidth
            ColumnLayout {
                width:parent.width; spacing:12
                RowLayout {
                    visible:dialog.globalScope
                    Layout.fillWidth:true
                    Label { text:qsTranslate("Vetust", "Custom executable"); Layout.preferredWidth:185 }
                    TextField { id:pathField; objectName:"customExecutable"; Layout.fillWidth:true; placeholderTextColor:"#91a3b3"; placeholderText:qsTranslate("Vetust", "Empty: automatic detection"); selectByMouse:true; onTextEdited: dialog.paths[dialog.editEngine]=text }
                }
                Label { Layout.fillWidth:true; wrapMode:Text.WordWrap; text:qsTranslate("Vetust", "Original profile → Global → This game. Changing the engine or hardware requires testing the game. Its sound menus may need matching settings."); color:"#c6b08d"; font.pixelSize:11 }
                Repeater {
                    id: fields
                    delegate: ColumnLayout {
                        id: fieldRow
                        required property var modelData
                        Layout.fillWidth:true; spacing:3
                        RowLayout {
                            Layout.fillWidth:true
                            Label { text:fieldRow.modelData.label; Layout.preferredWidth:185 }
                            ComboBox {
                                visible:fieldRow.modelData.choices.length>0
                                Layout.fillWidth:true
                                id:advancedChoice
                                model:[""].concat(fieldRow.modelData.choices)
                                displayText:currentIndex===0 ? qsTranslate("Vetust", "Inherit") : currentText
                                delegate:ItemDelegate { required property string modelData; width:advancedChoice.width; text:modelData==="" ? qsTranslate("Vetust", "Inherit") : modelData }
                                currentIndex:Math.max(0,model.indexOf(dialog.value(fieldRow.modelData.key)))
                                onActivated:dialog.setValue(fieldRow.modelData.key,currentIndex===0?"":currentText)
                            }
                            TextField {
                                visible:fieldRow.modelData.choices.length===0
                                Layout.fillWidth:true
                                placeholderTextColor:"#91a3b3"; placeholderText:qsTranslate("Vetust", "Inherit · or enter auto, max, 10000…")
                                text:dialog.value(fieldRow.modelData.key); selectByMouse:true
                                onTextEdited:dialog.setValue(fieldRow.modelData.key,text)
                            }
                        }
                        Label { Layout.leftMargin:195; Layout.fillWidth:true; wrapMode:Text.WordWrap; text:qsTranslate("Vetust", "When inherited: ")+(dialog.inherited[fieldRow.modelData.key] || ""); color:"#91a3b3"; font.pixelSize:11 }
                    }
                }

            }
        }
        Label { visible:dialog.error!==""; text:dialog.error; color:"#ffb1a4"; wrapMode:Text.WordWrap; Layout.fillWidth:true }
        RowLayout {
            Layout.fillWidth:true
            Button { text:dialog.globalScope?qsTranslate("Vetust", "Reset global settings"):qsTranslate("Vetust", "Inherit everything"); onClicked:{ dialog.draft={engine:dialog.globalScope?"staging":"",options:{}}; dialog.refresh() } }
            Item { Layout.fillWidth:true }
            Button { text:qsTranslate("Vetust", "Cancel"); onClicked:dialog.close() }
            Button { text:qsTranslate("Vetust", "Save"); highlighted:true; onClicked: {
                dialog.error=vetust.saveEmulatorSettings(dialog.globalScope,dialog.snapshot.gameId,dialog.draft,dialog.paths)
                if(dialog.error==="")dialog.close()
            } }
        }
    }
}
