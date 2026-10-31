import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog
    signal storageRequested()
    objectName: "emulatorDialog"
    anchors.centerIn: parent
    width: Math.min(880,parent.width-50)
    height: Math.min(advancedExpanded ? 780 : 450,parent.height-50)
    modal: true
    closePolicy: Popup.CloseOnEscape
    palette.window: "#111e29"; palette.windowText: "#e7edf1"
    palette.text: "#e7edf1"; palette.base: "#0b151f"
    palette.button: "#233443"; palette.buttonText: "#e7edf1"
    palette.highlight: "#377f87"; palette.highlightedText: "white"
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
        let choices=[{id:"",label:"Automático (según el emulador)"}].concat(vetust.monitors)
        let saved=draft.monitor || ""
        if(saved!=="" && !choices.some(m=>m.id===saved)) choices.push({id:saved,label:saved+" · Desconectado"})
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
        engineChoices=global ? snapshot.engines : [{id:"",name:"Heredar motor global",available:true}].concat(snapshot.engines)
        title=global ? "Ajustes globales de emulación" : "Emulación · " + vetust.selected.title
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
        fields.model=vetust.emulatorFields(editEngine).filter(f=>f.key!=="sdl.fullscreen")
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
        return e && !e.available ? "Este emulador no está disponible. " + (dialog.globalScope ? "Indica su ejecutable en Opciones avanzadas." : "Revisa Ajustes globales.") : ""
    }
    contentItem: ColumnLayout {
        spacing: 12
        Label { Layout.fillWidth:true; wrapMode:Text.WordWrap; text: dialog.globalScope ? "Elige cómo se abren tus juegos. Cada motor conserva sus propias opciones." : "Personaliza cómo se abre este juego. Las opciones avanzadas permiten ajustar su emulación."; color:"#a8bac8" }
        ColumnLayout {
            objectName:"monitorSection"
            visible:dialog.globalScope && vetust.monitors.length>1
            Layout.fillWidth:true; spacing:4
            RowLayout {
                Layout.fillWidth:true
                Label { text:"Monitor de los juegos"; Layout.preferredWidth:185 }
                ComboBox {
                    id:monitorPicker; objectName:"monitorPicker"; Layout.fillWidth:true
                    model:dialog.monitorChoices; textRole:"label"
                    onActivated:dialog.draft.monitor=dialog.monitorChoices[currentIndex].id
                }
            }
            Label {
                Layout.leftMargin:195; Layout.fillWidth:true; wrapMode:Text.WordWrap; font.pixelSize:11; color:"#91a3b3"
                text:"Se aplica a los juegos y a la consola DOS. Si el monitor se desconecta, se usa uno disponible."
            }
        }
        RowLayout {
            Label { text: "Motor"; Layout.preferredWidth: 185 }
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
            Label { text:"Pantalla completa"; Layout.preferredWidth:185 }
            Switch {
                objectName:"fullscreenSwitch"
                checked:dialog.value("sdl.fullscreen")!=="" ? dialog.value("sdl.fullscreen")==="true" : (dialog.inherited["sdl.fullscreen"] || "").startsWith("true")
                text:checked ? "Activada" : "Desactivada"
                Accessible.name:"Pantalla completa"
                onToggled:dialog.setValue("sdl.fullscreen",checked ? "true" : "false")
            }
            Item { Layout.fillWidth:true }
        }
        ToolButton {
            objectName:"advancedToggle"
            text:(dialog.advancedExpanded ? "▾ " : "▸ ")+"Opciones avanzadas"
            onClicked:dialog.advancedExpanded=!dialog.advancedExpanded
            Accessible.name:"Opciones avanzadas"
            Accessible.description:dialog.advancedExpanded ? "Desplegadas" : "Contraídas"
        }
        Button { visible:dialog.globalScope; text:"Biblioteca y carpetas…"; onClicked:dialog.storageRequested() }
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
                    Label { text:"Ejecutable personalizado"; Layout.preferredWidth:185 }
                    TextField { id:pathField; objectName:"customExecutable"; Layout.fillWidth:true; placeholderTextColor:"#91a3b3"; placeholderText:"Vacío: detección automática"; selectByMouse:true; onTextEdited: dialog.paths[dialog.editEngine]=text }
                }
                Label { Layout.fillWidth:true; wrapMode:Text.WordWrap; text:"Perfil original → Global → Este juego. Cambiar de motor o de hardware requiere comprobar el juego. Sus menús de sonido pueden necesitar la misma selección."; color:"#c6b08d"; font.pixelSize:11 }
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
                                model:["Heredar"].concat(fieldRow.modelData.choices)
                                currentIndex:Math.max(0,model.indexOf(dialog.value(fieldRow.modelData.key)))
                                onActivated:dialog.setValue(fieldRow.modelData.key,currentIndex===0?"":currentText)
                            }
                            TextField {
                                visible:fieldRow.modelData.choices.length===0
                                Layout.fillWidth:true
                                placeholderTextColor:"#91a3b3"; placeholderText:"Heredar · o escribe auto, max, 10000…"
                                text:dialog.value(fieldRow.modelData.key); selectByMouse:true
                                onTextEdited:dialog.setValue(fieldRow.modelData.key,text)
                            }
                        }
                        Label { Layout.leftMargin:195; Layout.fillWidth:true; wrapMode:Text.WordWrap; text:"Al heredar: "+(dialog.inherited[fieldRow.modelData.key] || ""); color:"#91a3b3"; font.pixelSize:11 }
                    }
                }

            }
        }
        Label { visible:dialog.error!==""; text:dialog.error; color:"#ffb1a4"; wrapMode:Text.WordWrap; Layout.fillWidth:true }
        RowLayout {
            Layout.fillWidth:true
            Button { text:dialog.globalScope?"Restablecer ajustes globales":"Heredar todo"; onClicked:{ dialog.draft={engine:dialog.globalScope?"staging":"",options:{}}; dialog.refresh() } }
            Item { Layout.fillWidth:true }
            Button { text:"Cancelar"; onClicked:dialog.close() }
            Button { text:"Guardar"; highlighted:true; onClicked: {
                dialog.error=vetust.saveEmulatorSettings(dialog.globalScope,dialog.snapshot.gameId,dialog.draft,dialog.paths)
                if(dialog.error==="")dialog.close()
            } }
        }
    }
}
