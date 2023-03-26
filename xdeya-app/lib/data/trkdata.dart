
import 'dart:io';
import 'dart:ui' as ui;
import 'package:flutter/services.dart' show rootBundle;
import 'package:flutter/material.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter_file_dialog/flutter_file_dialog.dart';
import 'package:file_picker/file_picker.dart';
import 'dart:developer' as developer;

import 'trklist.dart';
import '../net/proc.dart';
import 'logitem.dart';
import 'trktypes.dart';


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
    final List<LogItem> _data = [];
    List<LogItem> get data => _data;
    final List<TrkSeg> _seg = [];
    List<TrkSeg> get seg => _seg;

    // area
    TrkArea ?_area;
    TrkArea ? get area => _area;

    // trkCenter
    final ValueNotifier<LogItem ?> _center = ValueNotifier(null);
    LogItem ? get center => _center.value;
    ValueNotifier<LogItem ?> get notifyCenter => _center;

    // mapImg
    ui.Image ?_img;
    ui.Image ? get img => _img;
    Future<void> loadImg() async {
        _img = null;
        final data = await rootBundle.load('assets/map.jpg');
        _img = await decodeImageFromList(data.buffer.asUint8List());
        _sz.value ++;
        _sz.value --;
    }

    bool get isRecv => net.recieverContains({ 0x54, 0x55, 0x56 });
    Future<bool> Function() ?_reload;
    Future<bool> reload() async {
        return
            _reload != null ?
                await _reload!() :
                false;
    }
    Future<bool> netRequest(TrkItem trk, { Function() ?onLoad, Function(LogItem) ?onCenter }) async {
        _reload = () => netRequest(trk, onLoad:onLoad, onCenter:onCenter);
        loadImg();

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
                _seg.clear();
                _area = null;
                _sz.value = 0;
                _center.value = null;
                net.progmax = (_inf.fsize-32) ~/ 64;
            },
            item: (pro) {
                List<dynamic> ?v = pro.rcvData(pkLogItem);
                if ((v == null) || v.isEmpty) {
                    return;
                }

                final ti = LogItem.byvars(v);
                _data.add(ti);
                if (ti.satValid) {
                    if (_area == null) {
                        _area = TrkArea(ti);
                    }
                    else {
                        _area!.add(ti);
                    }
                }
                _sz.value = _data.length;
                net.progcnt = _data.length;

                if (
                    _seg.isNotEmpty &&
                    _seg.last.addAllow(ti)
                    ) {
                    _seg.last.data.add(ti);
                }
                else {
                    _seg.add(TrkSeg.byEl(ti));
                }

                if ((_center.value == null) && ti.satValid) {
                    _center.value = ti;
                    if (onCenter != null) onCenter(ti);
                }
            },
            end: (pro) {
                pro.rcvNext();
                developer.log('trkdata end ${_data.length}');
                net.progmax = 0;
                if (onLoad != null) onLoad();
            }
        );
    }

    String get exportJSON {
        String features = '';
        int segid = 0;
        for (final s in _seg) {
            if (!s.satValid) continue;

            for (int i=0; (i+1)<s.data.length; i+=5) {
                final p = s.data[i];
                String crd = p.crd;
                for (int n=1; ((i+n)<s.data.length) && (n < 5); n++) {
                    crd = '$crd,${s.data[i+n].crd}';
                }

                segid ++;

                final String horz = '${p['heading']}&deg; (${ p.hspeed.toStringAsFixed(1) } m/s)';
            
                features = 
                    '$features'
                    '{'
                        '"type": "Feature",'
                        '"id": $segid,'
                        '"options": {"strokeWidth": 4, "strokeColor": "${s.color}"},'
                        '"geometry": {'
                            '"type": "LineString",'
                            '"coordinates": [$crd]'
                        '},'
                        '"properties": {'
                            '"balloonContentHeader": "${p.time}",'
                            '"balloonContentBody": "Вертикаль: ${p.dscrVert}<br />Горизонт: $horz${p.dscrKach}",'
                            '"hintContent": "${p.time}<br/>Вер: ${p.dscrVert}<br/>Гор: $horz${p.dscrKach}",'
                        '}'
                    '},';
            }
        }

        return '{ "type": "FeatureCollection", "features": [ $features ] }';
    }

    String get exportGPX {
        List<String> data = [];

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
        for (final s in _seg) {
            if (!s.satValid) continue;
            
            data.add('<trkseg>');
            for (final p in s.data) {
                data.add(
                    '<trkpt lon="${p.lon}" lat="${p.lat}">'
                        '<name>${p.time}, ${p.dscrVert}</name>'
                        '<desc>Горизонт: ${p.dscrHorz}${p.dscrKach}</desc>'
                        '<ele>${p['alt']}</ele>'
                        '<magvar>${p['heading']}</magvar>'
                        '<sat>${p['sat']}</sat>'
                    '</trkpt>'
                );
            }
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
