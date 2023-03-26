import 'dart:math';
import 'package:vector_math/vector_math_64.dart';

import 'logitem.dart';
import 'dtime.dart';

/*//////////////////////////////////////
 *
 *  TrkInfo
 * 
 *//////////////////////////////////////
 
class TrkInfo {
    final int id;
    final int flags;
    final int jmpnum;
    final int jmpkey;
    final DateTime tmbeg;
    final int fsize;
    final int chksum;

    TrkInfo({
        required this.id,
        required this.flags,
        required this.jmpnum,
        required this.jmpkey,
        required this.tmbeg,
        required this.fsize,
        required this.chksum,
    });

    TrkInfo.byvars(List<dynamic> vars) :
        this(
            id:     (vars.isNotEmpty) && (vars[0]) is int ? vars[0] : 0,
            flags:  (vars.length > 1) && (vars[1]) is int ? vars[1] : 0,
            jmpnum: (vars.length > 2) && (vars[2]) is int ? vars[2] : 0,
            jmpkey: (vars.length > 3) && (vars[3]) is int ? vars[3] : 0,
            tmbeg:  (vars.length > 4) && (vars[4]) is DateTime ? vars[4] : DateTime(0),
            fsize:  (vars.length > 5) && (vars[5]) is int ? vars[5] : 0,
            chksum: (vars.length > 6) && (vars[6]) is int ? vars[6] : 0,
        );

    String get dtBeg => dt2format(tmbeg);
}


class TrkSeg {
    final String state;
    final bool satValid;
    final List<LogItem> data = [];

    TrkSeg(this.state, this.satValid);
    TrkSeg.byEl(LogItem ti) :
        state = ti['state'],
        satValid = ti.satValid;
    
    String get color =>
                (state == 's') || (state == 't') ? // takeoff
                    "#2e2f30" :
                state == 'f' ? // freefall
                    "#7318bf" :
                (state == 'c') || (state == 'l') ? // canopy
                    "#0052ef" :
                    "#fcb615";
    
    bool addAllow(LogItem ti) {
        return (state == ti['state']) &&
                (satValid == ti.satValid);
    }
}

class TrkArea {
    int _lonMin, _lonMax, _latMin, _latMax, _altMin, _altMax;

    TrkArea(LogItem log) :
        _lonMin = log['lon'],
        _lonMax = log['lon'],
        _latMin = log['lat'],
        _latMax = log['lat'],
        _altMin = log['alt'],
        _altMax = log['alt'];
    
    double get lonMin   => _lonMin / 10000000;
    double get lonCenter=> ((_lonMax - _lonMin) / 2 + _lonMin) / 10000000;
    double get lonMax   => _lonMax / 10000000;
    double get lonKoef  => cos(radians(latCenter));
    double get latMin   => _latMin / 10000000;
    double get latCenter=> ((_latMax - _latMin) / 2 + _latMin) / 10000000;
    double get latMax   => _latMax / 10000000;
    // является ли трек вытянутым вертикально (более высоким, чем широким)
    bool get isMapVertical => ((lonMax - _lonMin).abs() * lonKoef) < (latMax - _latMin).abs();
    int    get altMin   => _altMin;
    int    get altMax   => _altMax;

    void add(LogItem log) {
        if (!log.satValid) {
            return;
        }
        if (_latMin > log['lat']) {
            _latMin = log['lat'];
        }
        if (_latMax < log['lat']) {
            _latMax = log['lat'];
        }
        if (_lonMin > log['lon']) {
            _lonMin = log['lon'];
        }
        if (_lonMax < log['lon']) {
            _lonMax = log['lon'];
        }
        if (_altMin > log['alt']) {
            _altMin = log['alt'];
        }
        if (_altMax < log['alt']) {
            _altMax = log['alt'];
        }
    }
}
