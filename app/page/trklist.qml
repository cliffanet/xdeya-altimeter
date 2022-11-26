import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import com.xdeya.app

Pane {

    ListView {
        id: lvTrkList
        anchors.fill: parent

        model: app.trklist
        currentIndex: -1
        highlightFollowsCurrentItem: true
        focus: true

        onCountChanged: {
            //console.log("Scroll to end.");
            positionViewAtEnd();
        }

        delegate: Component {
            Item {
                width: lvTrkList.width
                height: childrenRect.height

                Row {
                    padding: 5
                    spacing: 2
                    Label {
                        Layout.fillWidth: true
                        text: '<b>№ прыжка: ' + modelData.jmpnum + '</b>'
                    }
                    Label {
                        Layout.fillWidth: true
                        elide: Label.ElideRight
                        text: modelData.dtBeg
                        leftPadding: 30
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        lvTrkList.currentIndex = index;
                        console.log('selected: ', index, '=', modelData.jmpnum);
                        app.page = AppHnd.PageTrkView;
                        app.trkView(modelData.trk);
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

