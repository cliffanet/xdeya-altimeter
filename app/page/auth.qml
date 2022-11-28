import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import com.xdeya.app 1.0

Pane {
    Column {
        spacing: 40
        width: parent.width
        topPadding: 20

        BusyIndicator {
            visible: app.isInit
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Image {
            visible: !app.isInit && !app.isAuth
            source: "/icons/warning.png"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        ColumnLayout {
            visible: !app.isInit && app.isAuth
            spacing: 40
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter

            onVisibleChanged: if (visible) inpCode.forceActiveFocus()

            Label {
                Layout.fillWidth: true
                wrapMode: Label.Wrap
                horizontalAlignment: Qt.AlignHCenter
                text: "Введите код, отображаемый на экране:"
            }

            TextField {
                id: inpCode
                horizontalAlignment: Qt.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
                validator: RegularExpressionValidator { regularExpression: /^[0-9A-Fa-f]{0,4}$/ }
                font.bold: true
                font.pixelSize: 24
                font.letterSpacing: 5
                text: ""
                focus: true
                onTextEdited: app.authEdit(text)
            }
        }
    }
}

