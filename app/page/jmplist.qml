import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import com.xdeya.app

Pane {

    Connections {
        target: app
        function onJmpSelected(index) {
            console.log("lvLogBook selected: ", index);
            lvLogBook.currentIndex = index;
        }
    }

    ListView {
        id: lvLogBook
        anchors.fill: parent

        model: app.jmplist
        currentIndex: -1
        highlightFollowsCurrentItem: true
        focus: true

        onCountChanged: {
            //console.log("Scroll to end.");
            positionViewAtEnd();
        }

        delegate: Component {
            Item {
                width: lvLogBook.width
                //height: 40
                height: childrenRect.height

                Row {
                    //anchors.fill: parent
                    padding: 5
                    spacing: 2
                    Label {
                        Layout.fillWidth: true
                        text: '<b>№ ' + modelData.num + '</b>'
                    }
                    Label {
                        Layout.fillWidth: true
                        elide: Label.ElideRight
                        text: modelData.date
                        leftPadding: 30
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        lvLogBook.currentIndex = index;
                        console.log('selected: ', index, '=', modelData.num);
                        app.page = AppHnd.PageJmpInfo;
                        app.setJmpInfo(index);
                    }
                }
            }
        }
        highlight: Rectangle {
            color: 'grey'
            radius: 5
        }
    }
}

