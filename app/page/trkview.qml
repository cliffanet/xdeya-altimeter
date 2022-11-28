import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtWebView 1.15
import com.xdeya.app 1.0

Pane {
    anchors.fill: parent
    padding: 0

    Connections {
        target: app
        function onTrkHtmlLoad(html) {
            //console.log("webView html: ", html);
            webView.loadHtml(html);
        }
        function onTrkRunJS(code) {
            //console.log("webView js: ", code);
            webView.runJavaScript(code);
        }
    }

    WebView {
        id: webView
        anchors.fill: parent

        onLoadingChanged: function(loadRequest) {
            if (loadRequest.errorString)
                console.error(loadRequest.errorString);
        }
    }
}

