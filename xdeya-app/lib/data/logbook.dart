
import 'dart:collection';
import 'package:flutter/foundation.dart';
import 'dart:developer' as developer;

import 'dtime.dart';
import 'logitem.dart';
import 'trklist.dart';
import '../data/filebin.dart';
import '../net/proc.dart';
import '../net/binproto.dart';

/*//////////////////////////////////////
 *
 *  LogBook
 * 
 *//////////////////////////////////////

class LogBook {
    static const pk = 'NNT$pkLogItem$pkLogItem$pkLogItem$pkLogItem';

    final int num;
    final int key;
    final DateTime tm;
    final Struct toff;
    final Struct beg;
    final Struct cnp;
    final Struct end;

    LogBook({
        required this.num,
        required this.key,
        required this.tm,
        required this.toff,
        required this.beg,
        required this.cnp,
        required this.end
    });

    LogBook.byvars(List<dynamic> vars) :
        this(
            num: (vars.isNotEmpty) && (vars[0]) is int ? vars[0] : 0,
            key: (vars.length > 1) && (vars[1]) is int ? vars[1] : 0,
            tm:  (vars.length > 2) && (vars[2]) is DateTime ? vars[2] : DateTime(0),
            toff: fldUnpack(fldLogItem, vars, 3),
            beg:  fldUnpack(fldLogItem, vars, 3 + fldLogItem.length),
            cnp:  fldUnpack(fldLogItem, vars, 3 + fldLogItem.length * 2),
            end:  fldUnpack(fldLogItem, vars, 3 + fldLogItem.length * 3)
        );
    
    String get date => '${tm.day}.${tm.month.toString().padLeft(2,'0')}.${tm.year}';
    String get timeTakeoff => sec2time( (toff['tmoffset'] ?? 0) ~/ 1000 );
    String get dtBeg => dt2format(tm);
    int    get altBeg => beg['alt'] ?? 0;
    String get timeFF => sec2time(((cnp['tmoffset']??0) - (beg['tmoffset']??0)) ~/ 1000);
    int    get altCnp => cnp['alt'] ?? 0;
    String get timeCnp => sec2time(((end['tmoffset']??0) - (cnp['tmoffset']??0)) ~/ 1000);
}

/*//////////////////////////////////////
 *
 *  LogBook
 * 
 *//////////////////////////////////////
final logbook = DataLogBook();
class DataLogBook extends ListBase<LogBook> {
    final List<LogBook> _list;
    DataLogBook() : _list = [];

    final ValueNotifier<int> _sz = ValueNotifier(0);
    ValueNotifier<int> get notify => _sz;

    @override
    set length(int l) { 
        _list.length=l;
        _sz.value = _list.length;
    }
    @override
    int get length => _list.length;

    @override
    LogBook operator [](int index) => _list[index];
    @override
    void operator []=(int index, LogBook value) {
        _list[index]=value;
        _sz.value = 0;
        _sz.value = _list.length;
    }

    Future<bool> netRequest({ int beg = 50, int count = 50, Function() ?onLoad }) async {
        return net.requestList(
            0x31, 'NN', [beg, count],
            beg: (pro) {
                List<dynamic> ?v = pro.rcvData('NN');
                if ((v == null) || v.isEmpty) {
                    return;
                }

                developer.log('logbook beg ${v[0]}, ${v[1]}');
                _list.clear();
                _sz.value = 0;
                net.progmax = v[0] < v[1] ? v[0] : v[1];
            },
            item: (pro) {
                List<dynamic> ?v = pro.rcvData(LogBook.pk);
                if ((v == null) || v.isEmpty) {
                    return;
                }
                _list.add(LogBook.byvars(v));
                _sz.value = _list.length;
                net.progcnt = _list.length;
            },
            end: (pro) {
                pro.rcvNext();
                developer.log('logbook end');
                net.progmax = 0;
                if (onLoad != null) onLoad();
            }
        );
    }

    Future<bool> netRequestDefault() {
        return netRequest(
            onLoad: () => trklist.netRequest()
        );
    }

    Future<bool> loadFile({ required String file, Function() ?onLoad }) async {
        final fh = FileBin();

        _list.clear();
        _sz.value = 0;

        if (!await fh.open(file)) {
            return false;
        }

        net.progmax = fh.size;

        while (!fh.eof) {
            final v = await fh.get(LogBook.pk);
            if (v == null) {
                break;
            }
            _list.add(LogBook.byvars(v));
            net.progcnt = fh.pos;
            _sz.value = fh.pos;
        }
        
        net.progmax = 0;
        _sz.value = -1;

        if (onLoad != null) onLoad();
        return true;
    }
}
