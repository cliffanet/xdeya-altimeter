import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtWebView
import com.xdeya.app

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

