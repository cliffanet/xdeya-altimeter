import 'dart:ui';
import 'package:vector_math/vector_math_64.dart' hide Colors;

import 'trktypes.dart';
import 'logitem.dart';

class TrkAreaView2D {
    final TrkArea area;
    final Size view;
    final double _mapx, _mapy;

    TrkAreaView2D(this.area, this.view) :
        _mapx = view.width / (area.lonMax - area.lonMin) / area.lonKoef,
        _mapy = view.height / (area.latMax - area.latMin);

    double get mapx => mapy * area.lonKoef;
    double get mapy => _mapx < _mapy ? _mapx : _mapy;

    double get padx => (view.width  - ((area.lonMax - area.lonMin) * mapx).abs()) / 2;
    double get pady => (view.height - ((area.latMax - area.latMin) * mapy).abs()) / 2;
}

class TrkPointTransform {
    final double begLon;
    final double begLat;
    final double begx;
    final double begy;
    final double begz;
    final double mapx;
    final double mapy;
    final double mapz;
    final double height;

    final double lonCenter;
    final double latCenter;
    final double xCenter;
    final double yCenter;

    TrkPointTransform(TrkArea area, Size view) :
        begLon = -area.lonMin,
        begLat = -area.latMin,
        begx = TrkAreaView2D(area, view).padx,
        begy = TrkAreaView2D(area, view).pady,
        begz = -((area.altMax - area.altMin) / 2),
        mapx = TrkAreaView2D(area, view).mapx,
        mapy = TrkAreaView2D(area, view).mapy,
        mapz = view.longestSide / (area.altMax - area.altMin),
        height = view.height,

        lonCenter   = area.lonCenter,
        latCenter   = area.latCenter,
        xCenter     = view.width / 2,
        yCenter     = view.height / 2;

    Vector3 mapLog(LogItem ti) {
        return
            Vector3(
                (ti.lon + begLon) * mapx + begx,
                height - (ti.lat + begLat) * mapy - begy,
                (ti.alt + begz) * mapz
            );
    }

    double mapLon(double lon) => (lon - lonCenter) * mapx + xCenter;
    double mapLat(double lat) => height - ((lat - latCenter) * mapy + yCenter);
}

/*//////////////////////////////////////
 *
 *  TrkViewPoint
 * 
 *//////////////////////////////////////
class TrkViewPoint {
    final Vector3 pnt;

    final LogItem ti;

    TrkViewPoint.byLogItem(LogItem log, TrkPointTransform transform) :
        ti = log,
        pnt = transform.mapLog(log);

    dynamic operator [](String f) => ti[f];
}

class TrkViewSeg {
    final String state;
    final List<TrkViewPoint> pnt;

    TrkViewSeg.bySeg(TrkSeg seg, TrkPointTransform transform) :
        state = seg.state,
        pnt = seg.data.map((p) => TrkViewPoint.byLogItem(p, transform)).toList();
    
    Color get color => trkColor(state);
}

Color trkColor(String state) {
    return
        (state == 's') || (state == 't') ? // takeoff
            const Color.fromRGBO(0x2e, 0x2f, 0x30, 1.0) :
        state == 'f' ? // freefall
            const Color.fromRGBO(0x73, 0x18, 0xbf, 1.0) :
        (state == 'c') || (state == 'l') ? // canopy
            const Color.fromRGBO(0x00, 0x52, 0xef, 1.0) :
            const Color.fromRGBO(0xfc, 0xb6, 0x15, 1.0);
}


