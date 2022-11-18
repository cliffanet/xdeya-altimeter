import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls
//import QtWebView
import QtQuick.Layouts

ApplicationWindow {
    id: app
    width: 640
    height: 480
    visible: true
    title: qsTr("Xde-Ya")

    property real hideValue: 0
    Behavior on hideValue {
        NumberAnimation { duration: 200 }
    }


    header: ToolBar {
        id: menu

        RowLayout {
            spacing: 20
            anchors.fill: parent

            ToolButton {
                id: btnBack
                icon.name: /*stack.depth > 1 ? "back" : */"drawer"
                visible: stack.depth > 1
                Layout.preferredWidth: menu.height
            }

            ProgressBar {
                Layout.fillWidth: true
            }

            Label {
                id: titleLabel
                text: "Gallery Text Long Text"
                //font.pixelSize: 20
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignRight
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
            }

            ToolButton {
                id: btnReload
                icon.name: "reload"
                Layout.preferredWidth: menu.height
            }
        }
    }

    StackView {
        id: stack
        anchors.fill: parent
    }
}
