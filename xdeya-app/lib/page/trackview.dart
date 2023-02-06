import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:webview_flutter/webview_flutter.dart';
import 'dart:developer' as developer;

import '../net/proc.dart';

class PageTrackView extends StatelessWidget {
    const PageTrackView({ super.key });
    
    @override
    Widget build(BuildContext context) {
        switch (Theme.of(context).platform) {
            case TargetPlatform.android:
            case TargetPlatform.iOS:
                return ValueListenableBuilder(
                    valueListenable: net.notifyTrkCenter,
                    builder: (BuildContext context, center, Widget? child) {
                        if (center == null) {
                            return const Center(
                                child: Text(
                                    'Нет координат трека', 
                                    style: TextStyle(color: Colors.black26)
                                )
                            );
                        }

                        double lat = center['lat'] / 10000000;
                        double lon = center['lon'] / 10000000;
                        developer.log('mapcenter: $lat, $lon');

                        return WebView(
                            javascriptMode: JavascriptMode.unrestricted,
                            initialUrl: 'about:blank',
                            onWebViewCreated: (WebViewController ctrl) async {
                                try {
                                    String file = await rootBundle.loadString('assets/navi.html');

                                    await ctrl.loadUrl(
                                        Uri.dataFromString(
                                            file.replaceFirst('%MAP_CENTER%', '$lat, $lon'),
                                            mimeType: 'text/html',
                                            encoding: Encoding.getByName('utf-8')
                                        ).toString()
                                    );
                                    
                                    developer.log('ctrl wait data');
                                    await Future.doWhile(
                                        () async {
                                            // без этой паузы повисает.
                                            await Future.delayed(const Duration(milliseconds: 100));
                                            return net.rcvElem.contains(NetRecvElem.trackdata);
                                        }
                                    );
                                    developer.log('ctrl fin');
                                    ctrl.runJavascript('loadGPX(${ net.trkGeoJson })');
                                }
                                catch (err) {
                                    developer.log('webview err: $err');
                                }
                            },
                        );
                    }
                );

            default:
        }

        return const Center(
            child: Text('TrackView')
        );
    }
}