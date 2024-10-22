import 'package:flutter/material.dart';
import 'titlebar.dart';

import 'page/discovery.dart';
import 'page/logbook.dart';
import 'page/jumpinfo.dart';
import 'page/tracklist.dart';
import 'page/trackview.dart';
import 'page/trackcube.dart';
import 'page/wifipass.dart';
import 'page/wifiedit.dart';
import 'page/filesbackup.dart';
import 'page/filesrestore.dart';
import 'page/chart.dart';
import 'data/logbook.dart';
import 'data/trklist.dart';
import 'data/trkdata.dart';
import 'data/wifipass.dart';
import 'net/wifidiscovery.dart';
import 'net/proc.dart';

enum PageCode {
    discovery, logbook, jumpinfo,
    tracklist, trackview, trackcube,
    wifipass, wifiedit,
    filesbackup, filesrestore,
    chartalt
}

typedef PageFunc = Widget ? Function([int ?index]);

final Map<PageCode, PageFunc> _pageMap = {
    PageCode.discovery: ([int ?i]) { return const PageDiscovery(); },
    PageCode.logbook:   ([int ?i]) { return PageLogBook(); },
    PageCode.jumpinfo:  ([int ?index]) { return index != null ? PageJumpInfo(index: index) : null; },
    PageCode.tracklist: ([int ?i]) { return PageTrackList(); },
    PageCode.trackview: ([int ?i]) { return const PageTrackView(); },
    PageCode.trackcube: ([int ?i]) { return PageTrackCube(); },
    PageCode.wifipass:  ([int ?i]) { return PageWiFiPass(); },
    PageCode.wifiedit:  ([int ?index]) { return index != null ? PageWiFiEdit(index: index) : null; },
    PageCode.filesbackup:([int ?i]) { return PageFilesBackup(); },
    PageCode.filesrestore:([int ?i]) { return const PageFilesRestore(); },
    PageCode.chartalt:  ([int ?i]) { return const PageChart(); },
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
            body: widget ?? Container(),
        );
    }

    static final List<PageCode> _stack = [];
    static PageCode? get top => _stack.isNotEmpty ? _stack[ _stack.length-1 ] : null;
    static bool get isEmpty => _stack.isEmpty;
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
        net.errClear();
        net.doNotifyInf();
    }
    static pop(BuildContext context) {
        Navigator.pop(context);
        _stack.removeLast();
        if (_stack.isEmpty) {
            net.stop();
            wifi.autosrch();
        }
        net.errClear();
        net.doNotifyInf();
    }

    static String get title {
        if (_stack.isEmpty) {
            return "";
        }

        switch (_stack.last) {
            case PageCode.logbook:  return "Прыжки";
            case PageCode.jumpinfo: return "Инфо о прыжке";
            case PageCode.tracklist:return "Треки";
            case PageCode.trackview:return "Трек на карте";
            case PageCode.trackcube:return "Трек на карте";
            case PageCode.wifipass: return "WiFi-пароли";
            //case PageCode.wifiedit: return "Изменение сети";
            case PageCode.filesbackup:return "Файлы";
            case PageCode.filesrestore:return "Восстановление";
            case PageCode.chartalt: return "График высоты";
            default:
        }

        return "";
    }

    static Function()? get refreshPressed {
        if (_stack.isEmpty || net.isLoading) {
            return null;
        }

        switch (_stack.last) {
            case PageCode.logbook:
                return () { logbook.netRequestDefault(); };
            
            case PageCode.tracklist:
                return () { trklist.netRequest(); };
            
            case PageCode.trackview:
            case PageCode.trackcube:
            case PageCode.chartalt:
                return () { trk.reload(); };
            
            case PageCode.wifipass:
                return () { wifipass.netRequest(); };

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
            case PageCode.trackcube:
            case PageCode.chartalt:
            case PageCode.wifipass:
                return true;

            default:
        }

        return false;
    }
}
