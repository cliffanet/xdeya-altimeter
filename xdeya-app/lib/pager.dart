import 'package:flutter/material.dart';
import 'titlebar.dart';

import 'page/discovery.dart';
import 'page/logbook.dart';
import 'page/jumpinfo.dart';
import 'page/tracklist.dart';
import 'page/trackview.dart';
import 'page/wifipass.dart';
import 'page/wifiedit.dart';
import '../net/proc.dart';

enum PageCode { discovery, logbook, jumpinfo, tracklist, trackview, wifipass, wifiedit }

typedef PageFunc = Widget ? Function([int ?index]);

final Map<PageCode, PageFunc> _pageMap = {
    PageCode.discovery: ([int ?i]) { return const PageDiscovery(); },
    PageCode.logbook:   ([int ?i]) { return PageLogBook(); },
    PageCode.jumpinfo:  ([int ?index]) { return index != null ? PageJumpInfo(index: index) : null; },
    PageCode.tracklist: ([int ?i]) { return PageTrackList(); },
    PageCode.trackview: ([int ?i]) { return const PageTrackView(); },
    PageCode.wifipass:  ([int ?i]) { return PageWiFiPass(); },
    PageCode.wifiedit:  ([int ?index]) { return index != null ? PageWiFiEdit(index: index) : null; },
};

class Pager extends StatelessWidget {
    final PageCode page;
    final int ?_index;
    const Pager({ super.key, required this.page, int ?index }) : _index = index;

    @override
    Widget build(BuildContext context) {
        PageFunc ?w = _pageMap[page];
        Widget ?widget = w != null ? w(_index) : null;

        return Scaffold(
            appBar: PreferredSize(
                preferredSize: const Size(double.infinity, kToolbarHeight), // here the desired height
                child:
                    page == PageCode.discovery ?
                        getTitleBarDiscovery() :
                        getTitleBarClient(page)
            ),
            body: widget ?? Container()
        );
    }

    static final List<PageCode> _stack = [];
    static PageCode? get top => _stack.isNotEmpty ? _stack[ _stack.length-1 ] : null;
    static push(BuildContext context, PageCode page, [int ?index]) {
        PageFunc ?w = _pageMap[page];
        if (w == null) {
            return;
        }

        Navigator.push(
            context,
            MaterialPageRoute(builder: (context) => Pager(page: page, index: index)),
        );
        _stack.add(page);
        net.doNotifyInf();
    }
    static pop(BuildContext context) {
        Navigator.pop(context);
        _stack.removeLast();
        if (_stack.isEmpty) {
            net.stop();
        }
        net.doNotifyInf();
    }

    static Function()? get refreshPressed {
        if (_stack.isEmpty || net.isLoading) {
            return null;
        }

        switch (_stack.last) {
            case PageCode.logbook:
                return () { net.requestLogBookDefault(); };
            
            case PageCode.tracklist:
                return () { net.requestTrkList(); };
            
            case PageCode.trackview:
                return () { net.reloadTrkData(); };
            
            case PageCode.wifipass:
                return () { net.requestWiFiPass(); };

            default:
        }

        return null;
    }

    static bool get refreshVisible {
        if (_stack.isEmpty) {
            return false;
        }

        switch (_stack.last) {
            case PageCode.logbook:
            case PageCode.tracklist:
            case PageCode.trackview:
            case PageCode.wifipass:
                return true;

            default:
        }

        return false;
    }
}
