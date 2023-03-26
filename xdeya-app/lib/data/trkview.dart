import 'dart:ui';
import 'package:vector_math/vector_math_64.dart' hide Colors;

import 'trktypes.dart';
import 'logitem.dart';

class TrkAreaView2D {
    final TrkArea area;
    final Size view;
    TrkAreaView2D(this.area, this.view);

    double get mapx => mapy * area.lonKoef;
    double get mapy =>
        view.shortestSide / (
            area.isMapVertical ?
                (area.latMax - area.latMin) :
                (area.lonMax - area.lonMin) / area.lonKoef
        );

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

    TrkPointTransform(TrkArea area, Size view) :
        begLon = -area.lonMin,
        begLat = -area.latMin,
        begx = TrkAreaView2D(area, view).padx,
        begy = TrkAreaView2D(area, view).pady,
        begz = -((area.altMax - area.altMin) / 2),
        mapx = TrkAreaView2D(area, view).mapx,
        mapy = TrkAreaView2D(area, view).mapy,
        mapz = view.longestSide / (area.altMax - area.altMin),
        height = view.height;

    Vector3 map(LogItem ti) {
        return
            Vector3(
                (ti.lon + begLon) * mapx + begx,
                height - (ti.lat + begLat) * mapy - begy,
                (ti.alt + begz) * mapz
            );
    }
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
        pnt = transform.map(log);

    dynamic operator [](String f) => ti[f];
}

class TrkViewSeg {
    final String state;
    final List<TrkViewPoint> pnt;

    TrkViewSeg.bySeg(TrkSeg seg, TrkPointTransform transform) :
        state = seg.state,
        pnt = seg.data.map((p) => TrkViewPoint.byLogItem(p, transform)).toList();
    
    Color get color =>
                (state == 's') || (state == 't') ? // takeoff
                    Color.fromRGBO(0x2e, 0x2f, 0x30, 1.0) :
                state == 'f' ? // freefall
                    Color.fromRGBO(0x73, 0x18, 0xbf, 1.0) :
                (state == 'c') || (state == 'l') ? // canopy
                    Color.fromRGBO(0x00, 0x52, 0xef, 1.0) :
                    Color.fromRGBO(0xfc, 0xb6, 0x15, 1.0);
}


