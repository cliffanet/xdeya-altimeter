import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import com.xdeya.app
import com.xdeya.jmpinfo
import com.xdeya.trkinfo

Flickable {
    contentHeight: pane.implicitHeight
    flickableDirection: Flickable.AutoFlickIfNeeded

    ColumnLayout {
        id: pane
        anchors.fill: parent

        RowLayout {
            spacing: 20
            width: parent.width

            ToolButton {
                Layout.preferredWidth: height * 2
                text: "<<"
                enabled: app.validJmpInfo(jmpInfo.index - 1)
                onClicked: app.setJmpInfo(jmpInfo.index - 1)
            }

            Item {
                // spacer item
                Layout.fillWidth: true
            }

           ToolButton {
                Layout.preferredWidth: height * 2
                text: ">>"
                enabled: app.validJmpInfo(jmpInfo.index + 1)
                onClicked: app.setJmpInfo(jmpInfo.index + 1)
            }
        }

        GridLayout {
            columns: 2
            rowSpacing: 10
            columnSpacing: 10
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Layout.fillWidth: true;

            Label { text: "#" }
            Label {
                Layout.fillWidth: true
                text: jmpInfo.num
            }

            Label { text: "Длительность взлёта:"}
            Label {
                Layout.fillWidth: true
                text: jmpInfo.timeTakeoff
            }

            Label { text: "Время отделения:"}
            Label {
                Layout.fillWidth: true
                text: jmpInfo.dtBeg
            }

            Label { text: "Высота отделения:"}
            Label {
                Layout.fillWidth: true
                text: jmpInfo.altBeg
            }

            Label { text: "Время падения:"}
            Label {
                Layout.fillWidth: true
                text: jmpInfo.timeFF
            }

            Label { text: "Высота открытия:"}
            Label {
                Layout.fillWidth: true
                text: jmpInfo.altCnp
            }

            Label { text: "Время пилотирования:"}
            Label {
                Layout.fillWidth: true
                text: jmpInfo.timeCnp
            }
        }

        Column {
            id: trkBtnList
            spacing: 10
            anchors.horizontalCenter: parent.horizontalCenter
            topPadding: 10

            Repeater {
                id: myList
                model: jmpInfo.trklist
                focus: true

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 200
                    text: "Трэк: " + modelData.dtBeg
                    onClicked: {
                        app.page = AppHnd.PageTrkView;
                        app.trkView(modelData.trk);
                    }
                }
            }
        }

        Item {
            // spacer item
            Layout.fillHeight: true
        }
    }
}

