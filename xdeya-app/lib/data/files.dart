
import 'dart:collection';
import 'dart:io';
import 'package:flutter/foundation.dart';
import 'dart:developer' as developer;

import '../net/proc.dart';

/*//////////////////////////////////////
 *
 *  FileItem
 * 
 *//////////////////////////////////////
class FileItem {
    final String name;
    final String? path;
    final int size;
    bool checked = true;

    FileItem({
        required this.name,
        required this.size,
        this.path
    });

    FileItem.byvars(List<dynamic> vars) :
        size = (vars.isNotEmpty) && (vars[0]) is int ? vars[0] : '',
        name = (vars.length > 1) && (vars[1]) is String ? vars[1] : '',
        path = null;
}

/*//////////////////////////////////////
 *
 *  Backup Files
 * 
 *//////////////////////////////////////
final files = DataFiles();
class DataFiles extends ListBase<FileItem> {
    final List<FileItem> _list;
    DataFiles() : _list = [];

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
    FileItem operator [](int index) => _list[index];
    @override
    void operator []=(int index, FileItem value) {
        _list[index]=value;
        _sz.value = 0;
        _sz.value = _list.length;
    }

    void _clear() {
        _list.clear();
         _sz.value = _list.length;
    }

    bool _addrcvFile(String dir, Function() ?onLoad) {
        RandomAccessFile ?fh;
        return
            net.recieverList(
                0x5b,
                beg: (pro) {
                    net.recieverDel(0x5e);
                    List<dynamic> ?v = pro.rcvData('Ns');
                    if ((v == null) || (v.length < 2)) {
                        return;
                    }
                    developer.log('requestFiles beg file: ${v[1]} (${v[0]} bytes)');
                    _list.add(FileItem.byvars(v));
                    _sz.value = _list.length;

                    final file = File('$dir/${v[1]}');
                    if (file.existsSync()) {
                        file.deleteSync();
                    }
                    fh = file.openSync(mode: FileMode.writeOnly);
                },
                item: (pro) {
                    List<dynamic> ?v = pro.rcvData('B');
                    if ((v == null) || v.isEmpty || (v[0] is! Uint8List)) {
                        return;
                    }
                    final data = v[0] as Uint8List;
                    developer.log('requestFiles data size=${data.length}');
                    net.progcnt += data.length;
                    fh!.writeFromSync(data.toList());
                },
                end: (pro) {
                    pro.rcvNext();
                    fh!.close();
                    developer.log('requestFiles end file');
                    _addrcvFile(dir, onLoad);
                    _addrcvFin(onLoad);
                }
            );
    }

    bool _addrcvFin(Function() ?onLoad) {
        return
            net.recieverAdd(0x5e, (pro) {
                net.recieverDel(0x5b);
                net.recieverDel(0x5e);
                pro.rcvNext();
                developer.log('requestFiles finish');
                if (onLoad != null) onLoad();
                net.progmax = 0;
            });
    }

    Future<bool> netRequest({ required String dir, Function() ?onLoad }) async {
        if (net.recieverContains({ 0x5a, 0x5b, 0x5c, 0x5d, 0x5e })) {
            developer.log('files request reciever exists');
            return false;
        }
        _clear();
        return net.requestOne(
            0x5a, null, null,
            (pro) {
                List<dynamic> ?v = pro.rcvData('nN');
                if ((v == null) || v.isEmpty) {
                    return;
                }
                _clear();
                net.progmax = v[1];
                developer.log('requestFiles beg proc ${v[0]} / ${v[1]} bytes');

                _addrcvFile(dir, onLoad);
                _addrcvFin(onLoad);
            }
        );
    }

    Future<bool> netSave({ required List<FileItem> files, Function() ?onDone }) async {
        if (files.isEmpty) {
            return false;
        }

        if (!await net.autochk()) return false;

        bool ok = net.confirmerAdd(0x5a, (err) {
            if ((err == null) && (onDone != null)) onDone();
        });
        if (!ok) {
            return false;
        }

        if (!net.send(0x5a)) {
            net.confirmerDel(0x5a);
            return false;
        }
        net.doNotifyInf();

        for (var f in files) {
            if ((f.path == null) || f.path!.isEmpty) continue;
            final fh = await File(f.path ?? '').open();
            if (!net.send(0x5b, 'Ns', [fh.lengthSync(), f.name])) {
                net.confirmerDel(0x5a);
                return false;
            }

            while (fh.positionSync() < fh.lengthSync()) {
                if (!net.send(0x5c, 'B', [await fh.read(256)])) {
                    net.confirmerDel(0x5a);
                    return false;
                }
            }

            fh.close();
            if (!net.send(0x5d)) {
                net.confirmerDel(0x5a);
                return false;
            }
        }

        if (!net.send(0x5e)) {
            net.confirmerDel(0x5a);
            return false;
        }
        return true;
    }
}
