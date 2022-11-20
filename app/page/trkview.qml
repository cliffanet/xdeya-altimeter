import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtWebView
import com.xdeya.app

Page {
    anchors.fill: parent

    Connections {
        target: app
        onTrkHtmlLoad: {
            //console.log("webView html: ", html);
            webView.loadHtml(html);
        }
        onTrkRunJS: {
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

