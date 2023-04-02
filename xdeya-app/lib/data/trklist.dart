
import 'dart:collection';
import 'package:flutter/foundation.dart';
import 'dart:developer' as developer;

import 'dtime.dart';
import 'logbook.dart';
import '../net/proc.dart';

/*//////////////////////////////////////
 *
 *  TrkItem
 * 
 *//////////////////////////////////////

class TrkItem {
    final int id;
    final int flags;
    final int jmpnum;
    final int jmpkey;
    final DateTime tmbeg;
    final int fsize;
    final int fnum;

    TrkItem({
        required this.id,
        required this.flags,
        required this.jmpnum,
        required this.jmpkey,
        required this.tmbeg,
        required this.fsize,
        required this.fnum,
    });

    TrkItem.byvars(List<dynamic> vars) :
        this(
            id:     (vars.isNotEmpty) && (vars[0]) is int ? vars[0] : 0,
            flags:  (vars.length > 1) && (vars[1]) is int ? vars[1] : 0,
            jmpnum: (vars.length > 2) && (vars[2]) is int ? vars[2] : 0,
            jmpkey: (vars.length > 3) && (vars[3]) is int ? vars[3] : 0,
            tmbeg:  (vars.length > 4) && (vars[4]) is DateTime ? vars[4] : DateTime(0),
            fsize:  (vars.length > 5) && (vars[5]) is int ? vars[5] : 0,
            fnum:   (vars.length > 6) && (vars[6]) is int ? vars[6] : 0,
        );

    String get dtBeg => dt2format(tmbeg);
}


/*//////////////////////////////////////
 *
 *  TrkList
 * 
 *//////////////////////////////////////
final trklist = DataTrkList();
class DataTrkList extends ListBase<TrkItem> {
    final List<TrkItem> _list;
    DataTrkList() : _list = [];

    final ValueNotifier<int> _sz = ValueNotifier(0);
    ValueNotifier<int> get notify => _sz;
    List<TrkItem> byJmp(LogBook jmp) {
        return _list.where((trk) => trk.jmpnum == jmp.num).toList();
    }

    @override
    set length(int l) { 
        _list.length=l;
        _sz.value = _list.length;
    }
    @override
    int get length => _list.length;

    @override
    TrkItem operator [](int index) => _list[index];
    @override
    void operator []=(int index, TrkItem value) {
        _list[index]=value;
        _sz.value = 0;
        _sz.value = _list.length;
    }

    Future<bool> netRequest({ Function() ?onLoad }) async {
        return net.requestList(
            0x51, null, null,
            beg: (pro) {
                pro.rcvNext();

                developer.log('trklist beg');
                _list.clear();
                _sz.value = 0;
            },
            item: (pro) {
                List<dynamic> ?v = pro.rcvData('NNNNTNC');
                if ((v == null) || v.isEmpty) {
                    return;
                }
                _list.add(TrkItem.byvars(v));
                _sz.value = _list.length;
            },
            end: (pro) {
                pro.rcvNext();
                developer.log('trklist end');
                if (onLoad != null) onLoad();
            }
        );
    }
}