import 'package:flutter/foundation.dart';
import 'dart:io';
import 'dart:async';
import 'dart:developer' as developer;

import '../data/logbook.dart';
import 'binproto.dart';
import 'types.dart';

enum NetState {
    offline,
    connecting,
    connected,
    waitauth,
    online
}

enum NetError {
    connect,
    disconnected,
    canceled,
    proto,
    cmddup,
    auth
}

enum NetRecvElem {
    logbook,
    tracklist,
    trackdata,
    wifipass,
    wifisave,
    files,
    filessave
}

final net = NetProc();

class NetProc {
    Socket ?_sock;
    StreamSubscription<Uint8List> ?_socklisten;
    Future<bool> ?_sockproc;

    /*//////////////////////////////////////
     *
     *  Общие даннные о состоянии
     * 
     *//////////////////////////////////////

    NetState _state = NetState.offline;
    NetState get state => _state;
    bool get isActive => (_state != NetState.offline);

    NetError? _err;
    NetError? get error => _err;
    void errClear() {
        _err = null;
        doNotifyInf();
    }

    Timer? _kalive;

    final Set<NetRecvElem> _rcvelm = {};
    Set<NetRecvElem> get rcvElem => _rcvelm;

    int _datacnt = 0;
    int _datamax = 0;
    set progmax(int v) { _datamax = v; _datacnt=0; }
    set progcnt(int v) => _datacnt = v;
    double get dataProgress => _datacnt > _datamax ? 1.0 : _datacnt / _datamax;
    bool get isProgress => _datamax > 0;

    final ValueNotifier<int> _notify = ValueNotifier(0);
    ValueNotifier<int> get notifyInf => _notify;
    void doNotifyInf() => _notify.value++;

    int _autokey = 0;
    InternetAddress ?_autoip;
    int ?_autoport;
    bool _sockchk() {
        if (_sock == null) {
            return false;
        }
        //if (_sock.) { // тут нужна проверка на валидность
        //  
        //}
        return true;
    }
    Future<bool> autochk() async {
        if (_sockchk()) {
            return true;
        }
        if ((_autokey == 0) ||
            (_autoip == null) ||
            (_autoport == null)) {
            return false;
        }

        return await start(_autoip!, _autoport!);
    }

    // start / stop

    void stop() {
        _errstop(NetError.canceled);
    }

    void _errstop(NetError ?err) {
        _clear();
        _err = err;
        doNotifyInf();
        developer.log('net stop by: $err');
    }

    void _clear() {
        if (_socklisten != null) {
            _socklisten!.cancel();
            _socklisten = null;
            developer.log('net subscription canceled');
        }
        if (_sockproc != null) {
            _sockproc!.ignore();
            _sockproc = null;
            developer.log('net await ignored');
        }
        if (_sock != null) {
            _sock!.close();
            _sock = null;
            developer.log('net sock closed');
        }

        _pro.rcvClear();
        _reciever.clear();
        _confirmer.clear();
        _rcvelm.clear();
        if (_kalive != null) {
            _kalive!.cancel();
        }
        _kalive = null;
        _datamax = 0;
        _datacnt = 0;

        _state = NetState.offline;
        developer.log('net cleared');
    }

    Future<bool> start(host, int port) {
        return _sockproc = _process(host, port);
    }

    Future<bool> _process(host, int port) async {
        stop();
        //_sock!.close();
        //await Future.doWhile(() => isActive);

        developer.log('net connecting to: $host:$port');
        _state = NetState.connecting;
        _err = null;
        doNotifyInf();

        try {
            _sock = await Socket.connect(host, port);
        }
        catch (err) {
            developer.log('net connect error: $err');
            _errstop(NetError.connect);
            return false;
        }

        if (_sock == null) {
            _errstop(NetError.connect);
            return false;
        }

        _socklisten = _sock!.listen(
            recv,
            onDone: ()  => _errstop(NetError.disconnected)
        );
        developer.log('net connected');

        _state = NetState.connected;
        doNotifyInf();

        // запрос hello
        if (!recieverAdd(0x02, (_) {
                recieverDel(0x02);
                // 0x03 (authreply) вы добавляем, если у нас есть
                // autokey с предыдущей авторизации, но он может не пройти
                recieverDel(0x03);
                _pro.rcvNext();
                _state = NetState.waitauth;
                _autokey = 0;
                _autoip = null;
                _autoport = null;
                developer.log('rcv hello');
            }))
        {
            _errstop(NetError.cmddup);
            return false;
        }

        if (!recieverAdd(0x03, _auth_recv)) {
            // чтобы можно было сделать тестовый вход без авторизации,
            // добавляем обработчик всегда, даже если нет _autokey
            _errstop(NetError.cmddup);
            return false;
        }

        if (_autokey > 0) {
            if (!send(0x02, 'n', [_autokey])) {
                return false;
            }
        }
        else {
            if (!send(0x02)) {
                return false;
            }
        }

        _kalive = Timer.periodic(
            const Duration(seconds: 20),
            (timer) { 
                send(0x05);
                developer.log('send keep-alive');
            }
        );

        return true;
    }

    /*//////////////////////////////////////
     *
     *  recv / send
     * 
     *//////////////////////////////////////

    final BinProto _pro = BinProto();
    void recv(data) {
        if (!_pro.rcvProcess(data)) {
            _errstop(NetError.proto);
            return;
        }

        while (_pro.rcvState == BinProtoRecv.complete) {
            if (_pro.rcvCmd == 0x05) {
                _pro.rcvNext();
                developer.log("recv keep-alive");
            }
            else
            if (_pro.rcvCmd == 0x10) {
                _recvConfirm();
            }
            else {
                Function(BinProto) ?hnd = _reciever[ _pro.rcvCmd ];

                if (hnd != null) {
                    hnd(_pro);
                    doNotifyInf();
                }
                else {
                    developer.log('recv unknown: cmd=0x${_pro.rcvCmd.toRadixString(16)}');
                    _pro.rcvNext();
                }
            }

            if (!_pro.rcvProcess()) {
                _errstop(NetError.proto);
                return;
            }
        }
    }

    bool _recvConfirm() {
        List<dynamic> ?v = _pro.rcvData('CC');
        if (
                (v == null) || (v.length < 2) ||
                (v[0] is! int) || (v[1] is! int)
            ) {
            return false;
        }

        int cmd = v[0];
        int err = v[1];
        Function(int ?err) ?hnd = _confirmer[ cmd ];
        if (hnd == null) {
            developer.log('confirm unknown: cmd=0x${cmd.toRadixString(16)}');
            return false;
        }

        developer.log('confirm: cmd=0x${cmd.toRadixString(16)}, err=$err');
        hnd(err == 0 ? null : err);
        confirmerDel(cmd);
        doNotifyInf();

        return true;
    }

    bool send(int cmd, [String? pk, List<dynamic>? vars]) {
        if (_sock == null)  {
            _err = NetError.disconnected;
            doNotifyInf();
            return false;
        }
        
        var data = _pro.pack(cmd, pk, vars);
        if (_sock != null) {
            _sock!.add(data);
        }
        developer.log('send cmd=$cmd, size=${ data.length }');

        return true;
    }

    /*//////////////////////////////////////
     *
     *  reciever
     * 
     *//////////////////////////////////////

    final Map<int, void Function(BinProto)> _reciever = {};
    bool get isLoading => _reciever.isNotEmpty || _confirmer.isNotEmpty;

    bool recieverAdd(int cmd, void Function(BinProto) hnd) {
        if (_sock == null)  {
            _err = NetError.disconnected;
            doNotifyInf();
            return false;
        }

        if (_reciever.containsKey(cmd)) {
            return false;
        }

        _reciever[cmd] = hnd;

        return true;
    }
    
    bool recieverDel(int cmd) {
        if (!_reciever.containsKey(cmd)) {
            return false;
        }

        _reciever.remove(cmd);

        return true;
    }

    void Function(BinProto)? reciever(int cmd) {
        if (!_reciever.containsKey(cmd)) {
            return null;
        }
        return _reciever[cmd];
    }

    bool recieverContains(Set<int> cmds) {
        for (final cmd in cmds) {
            if (_reciever.containsKey(cmd)) {
                return true;
            }
        }

        return false;
    }

    bool recieverList(int cmd, {
            void Function(BinProto) ?beg,
            void Function(BinProto) ?item,
            void Function(BinProto) ?end }) {

        return recieverAdd(cmd, (pro) {
            recieverDel(cmd);
            beg!(pro);

            if (item != null) {
                recieverAdd(cmd+1, item);
            }
            recieverAdd(cmd+2, (pro) {
                recieverDel(cmd+1);
                recieverDel(cmd+2);

                end!(pro);
            });
        });
    }

    /*//////////////////////////////////////
     *
     *  confirmer
     * 
     *//////////////////////////////////////

    final Map<int, void Function(int ?err)> _confirmer = {};

    bool confirmerAdd(int cmd, void Function(int ?err) hnd) {
        if (_confirmer.containsKey(cmd)) {
            return false;
        }

        _confirmer[cmd] = hnd;

        return true;
    }
    
    bool confirmerDel(int cmd) {
        if (!_confirmer.containsKey(cmd)) {
            return false;
        }

        _confirmer.remove(cmd);

        return true;
    }

    void Function(int ?err)? confirmer(int cmd) {
        if (!_confirmer.containsKey(cmd)) {
            return null;
        }

        return _confirmer[cmd];
    }


    /*//////////////////////////////////////
     *
     *  request
     * 
     *//////////////////////////////////////
    
    Future<bool> requestList(
            int cmd, String? pk, List<dynamic>? vars,
            {
                void Function(BinProto pro) ?beg,
                void Function(BinProto pro) ?item,
                void Function(BinProto pro) ?end,
            }
        ) async {
        if (_reciever.containsKey(cmd) || _reciever.containsKey(cmd+1) || _reciever.containsKey(cmd+2)) {
            developer.log('reciever($cmd or ${cmd+1} or ${cmd+2}) exists');
            return false;
        }

        if (!await autochk()) return false;
        
        bool ok =
            recieverList(cmd, beg: beg, item: item, end: end) &&
            send(0x0a, 'C${pk?? ''}', [cmd, ...(vars ?? [])]);

        if (!ok) recieverDel(cmd);

        return ok;
    }


    /*//////////////////////////////////////
     *
     *  Auth
     * 
     *//////////////////////////////////////

    bool _auth_recv(BinProto _) {
        recieverDel(0x02);
        recieverDel(0x03);
        List<dynamic> ?v = _pro.rcvData('Cn');
        if ((v == null) || v.isEmpty || (v[0] > 0)) {
            _errstop(NetError.auth);
            return false;
        }
        if ((v.length >= 2) && (v[1] > 0) && (_sock != null)) {
            _autokey    = v[1];
            _autoip     = _sock!.remoteAddress;
            _autoport   = _sock!.remotePort;
        }
        else {
            _autokey    = 0;
            _autoip     = null;
            _autoport   = null;
        }
        developer.log('auth ok');
        _state = NetState.online;

        logbook.netRequestDefault();
        
        return true;
    }
    bool requestAuth(String codehex, { Function() ?onReplyOk, Function() ?onReplyErr }) {
        int code = int.parse(codehex, radix: 16);
        if (code == 0) {
            return false;
        }

        bool ok = recieverAdd(0x03, (pro) {
                final on = _auth_recv(pro) ? onReplyOk : onReplyErr;
                if (on != null) on();
            });
        if (!ok) return false;
        doNotifyInf();

        return send(0x03, 'n', [code]);
    }
    

    /*//////////////////////////////////////
     *
     *  Backup Files
     * 
     *//////////////////////////////////////
    final ValueNotifier<int> notifyFilesList = ValueNotifier(0);
    final List<FileItem> files = [];
    void _filesClear() {
        files.clear();
        notifyFilesList.value = 0;
    }

    Future<bool> requestFiles({ required String dir, Function() ?onLoad }) async {
        if (!await autochk()) return false;

        if (_rcvelm.contains(NetRecvElem.files)) {
            return false;
        }
        _filesClear();

        bool ok = recieverAdd(0x5a, (_) {
                recieverDel(0x5a);
                List<dynamic> ?v = _pro.rcvData('nN');
                if ((v == null) || v.isEmpty) {
                    return;
                }
                _filesClear();
                _datamax = v[1];
                _datacnt = 0;

                void recvfin(_) {
                    recieverDel(0x5b);
                    recieverDel(0x5e);
                    _pro.rcvNext();
                    developer.log('requestFiles finish');
                    _rcvelm.remove(NetRecvElem.files);
                    if (onLoad != null) onLoad();
                    _datamax = 0;
                    _datacnt = 0;
                }
                recieverAdd(0x5e, recvfin);

                developer.log('requestFiles beg proc ${v[0]} / ${v[1]} bytes');
                void recvfile(_) async {
                    recieverDel(0x5b);
                    recieverDel(0x5e);
                    List<dynamic> ?v = _pro.rcvData('Ns');
                    if ((v == null) || (v.length < 2)) {
                        return;
                    }
                    developer.log('requestFiles beg file: ${v[1]} (${v[0]} bytes)');
                    files.add(FileItem.byvars(v));
                    notifyFilesList.value = files.length;

                    final file = File('$dir/${v[1]}');
                    if (file.existsSync()) {
                        file.deleteSync();
                    }
                    final fh = file.openSync(mode: FileMode.writeOnly);

                    recieverAdd(0x5c, (_) {
                        List<dynamic> ?v = _pro.rcvData('B');
                        if ((v == null) || v.isEmpty || (v[0] is! Uint8List)) {
                            return;
                        }
                        final data = v[0] as Uint8List;
                        developer.log('requestFiles data size=${data.length}');
                        _datacnt += data.length;
                        fh.writeFromSync(data.toList());
                    });
                    recieverAdd(0x5d, (_) {
                        recieverDel(0x5c);
                        recieverDel(0x5d);
                        _pro.rcvNext();
                        fh.close();
                        developer.log('requestFiles end file');
                        recieverAdd(0x5b, recvfile);
                        recieverAdd(0x5e, recvfin);
                    });
                }
                recieverAdd(0x5b, recvfile);
            });
        if (!ok) return false;
        if (!send(0x0a, "C", [0x5a])) {
            recieverDel(0x5a);
            return false;
        }
        _rcvelm.add(NetRecvElem.files);
        doNotifyInf();

        return true;
    }

    Future<bool> saveFiles({ required List<FileItem> files, Function() ?onDone }) async {
        if (!await autochk()) return false;

        if (_rcvelm.contains(NetRecvElem.filessave)) {
            return false;
        }
        if (files.isEmpty) {
            return false;
        }
        bool ok = confirmerAdd(0x5a, (err) {
            _rcvelm.remove(NetRecvElem.filessave);
            if ((err == null) && (onDone != null)) onDone();
        });
        if (!ok) {
            return false;
        }

        if (!send(0x5a)) {
            confirmerDel(0x5a);
            return false;
        }
        _rcvelm.add(NetRecvElem.filessave);
        doNotifyInf();

        for (var f in files) {
            if ((f.path == null) || f.path!.isEmpty) continue;
            final fh = await File(f.path ?? '').open();
            if (!send(0x5b, 'Ns', [fh.lengthSync(), f.name])) {
                confirmerDel(0x5a);
                return false;
            }

            while (fh.positionSync() < fh.lengthSync()) {
                if (!send(0x5c, 'B', [await fh.read(256)])) {
                    confirmerDel(0x5a);
                    return false;
                }
            }

            fh.close();
            if (!send(0x5d)) {
                confirmerDel(0x5a);
                return false;
            }
        }

        if (!send(0x5e)) {
            confirmerDel(0x5a);
            return false;
        }
        return true;
    }
}
