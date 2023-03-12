
import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:flutter_file_dialog/flutter_file_dialog.dart';
import 'package:file_picker/file_picker.dart';
import 'dart:developer' as developer;

import 'dtime.dart';
import 'trklist.dart';
import '../net/proc.dart';
import '../net/binproto.dart';
import '../net/types.dart';


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


/*//////////////////////////////////////
 *
 *  TrkData
 * 
 *//////////////////////////////////////
final trk = DataTrack();
class DataTrack {
    // trkinfo
    TrkInfo _inf = TrkInfo.byvars([]);
    TrkInfo get inf => _inf;

    // trkdata
    final ValueNotifier<int> _sz = ValueNotifier(0);
    ValueNotifier<int> get notifyData => _sz;
    final List<Struct> _data = [];
    List<Struct> get data => _data;
    bool satValid(int i) => ((_data[i]['flags'] ?? 0) & 0x0001) > 0;

    // trkCenter
    final ValueNotifier<Struct ?> _center = ValueNotifier(null);
    Struct ? get center => _center.value;
    ValueNotifier<Struct ?> get notifyCenter => _center;

    bool get isRecv => net.recieverContains({ 0x54, 0x55, 0x56 });
    Future<bool> Function() ?_reload;
    Future<bool> reload() async {
        return
            _reload != null ?
                await _reload!() :
                false;
    }
    Future<bool> netRequest(TrkItem trk, { Function() ?onLoad, Function(Struct) ?onCenter }) async {
        _reload = () => netRequest(trk, onLoad:onLoad, onCenter:onCenter);

        return net.requestList(
            0x54, 'NNNTC', [trk.id, trk.jmpnum, trk.jmpkey, trk.tmbeg, trk.fnum],
            beg: (pro) {
                List<dynamic> ?v = pro.rcvData('NNNNTNH');
                if ((v == null) || v.isEmpty) {
                    return;
                }
                _inf = TrkInfo.byvars(v);

                developer.log('trkdata beg ${_inf.jmpnum}, ${_inf.dtBeg}');
                _data.clear();
                _sz.value = 0;
                _center.value = null;
                net.progmax = (_inf.fsize-32) ~/ 64;
            },
            item: (pro) {
                List<dynamic> ?v = pro.rcvData(pkLogItem);
                if ((v == null) || v.isEmpty) {
                    return;
                }
                Struct ti = fldUnpack(fldLogItem, v);
                _data.add(ti);
                _sz.value = _data.length;
                net.progcnt = _data.length;
                if ((_center.value == null) && (((ti['flags'] ?? 0) & 0x0001) > 0)) {
                    _center.value = ti;
                    if (onCenter != null) onCenter(ti);
                }
            },
            end: (pro) {
                pro.rcvNext();
                developer.log('trkdata end');
                net.progmax = 0;
                if (onLoad != null) onLoad();
            }
        );
    }

    String get exportJSON {
        String features = '';
        int i=0;
        int segid = 0;
        while (i < _data.length) {
            while ( // пропустим все точки без спутников
                    (i < _data.length) &&
                    !satValid(i)
                ) {
                i++;
            }
            if (i >= _data.length) {
                break;
            }

            Struct p = _data[i];
            String pstate = p['state'];

            double lat = p['lat'] / 10000000;
            double lon = p['lon'] / 10000000;
            String crd = '[$lat,$lon]';

            int n = 1;
            i++;
            while (
                    (i < _data.length) &&
                    (n < 5) &&
                    (_data[i]['state'] == pstate) &&
                    satValid(i)
                ) {
                double lat = _data[i]['lat'] / 10000000;
                double lon = _data[i]['lon'] / 10000000;
                crd = '$crd,[$lat,$lon]';
                i++;
                n++;
            }

            if (n < 2) continue;

            segid ++;
            final String color =
                (pstate == 's') || (pstate == 't') ? // takeoff
                    "#2e2f30" :
                pstate == 'f' ? // freefall
                    "#7318bf" :
                (pstate == 'c') || (pstate == 'l') ? // canopy
                    "#0052ef" :
                    "#fcb615";
            

            int sec = (p['tmoffset'] ?? 0) ~/ 1000;
            int min = sec ~/ 60;
            sec -= min*60;
            final String time = '$min:${sec.toString().padLeft(2,'0')}';

            final String vert = '${p['alt']} m (${ (p['altspeed']/100).toStringAsFixed(1) } m/s)';
            final String horz = '${p['heading']}&deg; (${ (p['hspeed']/100).toStringAsFixed(1) } m/s)';

            final String kach =
                p['altspeed'] < -10 ?
                    ' [кач: ${ (-1.0 * p['hspeed'] / p['altspeed']).toStringAsFixed(1) }]' :
                    '';
            
            features = 
                '$features'
                '{'
                    '"type": "Feature",'
                    '"id": $segid,'
                    '"options": {"strokeWidth": 4, "strokeColor": "$color"},'
                    '"geometry": {'
                        '"type": "LineString",'
                        '"coordinates": [$crd]'
                    '},'
                    '"properties": {'
                        '"balloonContentHeader": "$time",'
                        '"balloonContentBody": "Вертикаль: $vert<br />Горизонт: $horz$kach",'
                        '"hintContent": "$time<br/>Вер: $vert<br/>Гор: $horz$kach",'
                    '}'
                '},';
        }

        return '{ "type": "FeatureCollection", "features": [ $features ] }';
    }

    String get exportGPX {
        List<String> data = [];
        int i = 0;
        int seg = 0;

        data.add(
            '<?xml version="1.0" encoding="UTF-8"?>'
            '<gpx version="1.1" creator="XdeYa altimeter" xmlns="http://www.topografix.com/GPX/1/1">'
                '<metadata>'
                    '<name><![CDATA[Без названия]]></name>'
                    '<desc/>'
                    '<time>2017-10-18T12:19:23.353Z</time>'
                '</metadata>'
                '<trk>'
                    '<name>трек</name>'
        );

        String pstate = '';

        while (i < _data.length) {
            if ((seg > 0) && (
                    !satValid(i) ||
                    (pstate != _data[i]['state'])
                )) {
                data.add('</trkseg>');
                seg = 0;
            }
            while ( // пропустим все точки без спутников
                    (i < _data.length) &&
                    !satValid(i)
                ) {
                i++;
            }
            if (i >= _data.length) {
                break;
            }

            if (seg == 0) {
                data.add('<trkseg>');
            }

            Struct p = _data[i];
            i++;
            seg++;
            pstate = p['state'];

            double lat = p['lat'] / 10000000;
            double lon = p['lon'] / 10000000;

            int sec = (p['tmoffset'] ?? 0) ~/ 1000;
            int min = sec ~/ 60;
            sec -= min*60;
            final String time = '$min:${sec.toString().padLeft(2,'0')}';

            final String vert = '${p['alt']} m (${ (p['altspeed']/100).toStringAsFixed(1) } m/s)';
            final String horz = '${p['heading']}гр (${ (p['hspeed']/100).toStringAsFixed(1) } m/s)';

            final String kach =
                p['altspeed'] < -10 ?
                    ' [кач: ${ (-1.0 * p['hspeed'] / p['altspeed']).toStringAsFixed(1) }]' :
                    '';

            data.add(
                '<trkpt lon="$lon" lat="$lat">'
                    '<name>$time, $vert</name>'
                    '<desc>Горизонт: $horz$kach</desc>'
                    '<ele>${p['alt']}</ele>'
                    '<magvar>${p['heading']}</magvar>'
                    '<sat>${p['sat']}</sat>'
                '</trkpt>'
            );
        }

        if (seg > 0) {
            data.add('</trkseg>');
        }

        data.add(
                '</trk>'
            '</gpx>'
        );

        return data.join();
    }

    Future<bool> saveGpx(String? filename) async {
        filename ??= 'track.gpx';
        final String data = exportGPX;
        
        if (Platform.isAndroid || Platform.isIOS) {
            final filePath = await FlutterFileDialog.saveFile(
                params: SaveFileDialogParams(
                    fileName: filename,
                    //sourceFilePath: '~'
                    data: Uint8List.fromList(data.codeUnits)
                )
            );
            developer.log('file: $filePath');
            return filePath != null;
        }
        else
        if (Platform.isLinux || Platform.isWindows || Platform.isMacOS) {
            final filePath = await FilePicker.platform.saveFile(
                dialogTitle: 'Please select an output file:',
                fileName: filename,
            );
            developer.log('file: $filePath');
            if (filePath == null) {
                return false;
            }

            final file = File(filePath);

            file.writeAsString(data);

            return true;
        }

        return false;
    }
}
