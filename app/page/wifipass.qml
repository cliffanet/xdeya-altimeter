import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import com.xdeya.app
import com.xdeya.jmpinfo
import com.xdeya.trkinfo

Flickable {
    contentHeight: pane.implicitHeight
    flickableDirection: Flickable.AutoFlickIfNeeded
    onVisibleChanged: if (visible) btnAccept.enabled = false;

    function doedit() {
        btnAccept.enabled = true;
    }

    ColumnLayout {
        id: pane
        anchors.fill: parent

        GridLayout {
            columns: 3
            rowSpacing: 10
            columnSpacing: 10
            Layout.margins: 10
            Layout.fillWidth: true

            Repeater {
                model: app.wifilist

                TextField {
                    Layout.row: index
                    Layout.column: 0
                    Layout.fillWidth: true
                    text: modelData.ssid
                    readOnly: true
                    selectByMouse: true
                    font.bold: readOnly
                    onPressAndHold: {
                        if (!readOnly)
                            return;
                        readOnly = false;
                        forceActiveFocus();
                    }
                    onEditingFinished: {
                        if (readOnly)
                            return;
                        if (text !== modelData.ssid)
                            doedit();
                        app.setWiFiSSID(index, text);
                        readOnly = true;
                    }
                    onTextEdited: doedit();
                }
            }

            Repeater {
                model: app.wifilist

                TextField {
                    Layout.row: index
                    Layout.column: 1
                    Layout.fillWidth: true
                    text: readOnly ? modelData.passstar : modelData.pass
                    readOnly: true
                    selectByMouse: true
                    onPressAndHold: {
                        if (!readOnly)
                            return;
                        readOnly = false;
                        forceActiveFocus();
                    }
                    onEditingFinished: {
                        if (readOnly)
                            return;
                        if (text !== modelData.pass)
                            doedit();
                        app.setWiFiPass(index, text);
                        readOnly = true;
                    }
                    onTextEdited: doedit();
                }
            }

            Repeater {
                model: app.wifilist

                Button {
                    Layout.row: index
                    Layout.column: 2
                    Layout.preferredWidth: height
                    text: "x"
                    onClicked: {
                        doedit();
                        app.delWiFiPass(index);
                    }
                }
            }
        }

        RowLayout {
            Layout.margins: 10
            spacing: 10

            Item {
                // spacer item
                Layout.fillWidth: true
            }

            Button {
                text: "Добавить"
                onClicked: app.addWiFiPass();
            }

            Button {
                id: btnAccept
                text: "Сохранить"
                enabled: false;
                onClicked: app.wifiPassSave();
            }
        }

        Item {
            // spacer item
            Layout.fillHeight: true
        }
    }
}

