import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls
//import QtWebView
import QtQuick.Layouts
import com.xdeya.app

ApplicationWindow {
    id: main
    width: 640
    height: 480
    minimumHeight: 200
    minimumWidth: 150
    visible: true
    title: qsTr("Xde-Ya")

    Connections {
        target: app
        onPagePushed: {
            console.log("page push: ", src);
            stack.push(src);
        }
        onPagePoped: {
            console.log("page poped");
            stack.pop();
        }
    }

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
                icon.name: stack.depth > 1 ? "back" : "drawer"
                visible: stack.depth > 1
                Layout.preferredWidth: menu.height
                onClicked: {
                    app.pageBack();
                    if (stack.depth <= 1)
                        app.devDisconnect();
                }
            }

            ProgressBar {
                Layout.fillWidth: true
                visible: app.progressEnabled
                indeterminate: app.progressMax === 0
                from: 0
                to: app.progressMax
                value: app.progressVal
            }

            Item {
                // spacer item
                Layout.fillWidth: true
            }

            Label {
                id: titleLabel
                text: app.txtState
                visible: app.txtState !== ""
                //font.pixelSize: 20
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignRight
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
            }

            ToolButton {
                id: btnReload
                visible: app.reloadVisibled
                enabled: app.reloadEnabled
                icon.name: "reload"
                Layout.preferredWidth: menu.height
                onClicked: app.clickReload();
            }
        }
    }

    StackView {
        id: stack
        anchors.fill: parent

        initialItem: Pane {
            id: paneDevSrch

            ListView {
                id: lvDevSrch
                anchors.fill: parent

                model: app.devlist
                currentIndex: -1
                highlightFollowsCurrentItem: true
                focus: true

                delegate: Component {
                    Item {
                        width: lvDevSrch.width
                        height: 40

                        Column {
                            width: parent.width
                            padding: 5
                            spacing: 2
                            Label {
                                width: parent.width - 10
                                elide: Label.ElideRight
                                text: '<b>' + modelData.name + '</b>'
                            }
                            Label {
                                width: parent.width - 10
                                elide: Label.ElideRight
                                text: 'Address: ' + modelData.address
                                leftPadding: 30
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                lvDevSrch.currentIndex = index;
                                console.log('selected: ', index, '=', modelData.name);
                                app.page = AppHnd.PageAuth;
                                app.devConnect(index);
                            }
                        }
                    }
                }
                highlight: Rectangle {
                    color: 'grey'
                    radius: 5
                }
                add: Transition {
                    NumberAnimation { properties: "x,y"; duration: 1000 }
                }
                displaced: Transition {
                    NumberAnimation { properties: "x,y"; duration: 1000 }
                }
            }
        }
    }
}