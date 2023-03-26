import 'package:flutter/foundation.dart';
import 'dart:io';
import 'dart:async';
import 'dart:developer' as developer;

import '../data/logbook.dart';
import 'binproto.dart';

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

    int progcnt = 0;
    int _datamax = 0;
    int get progmax => _datamax;
    set progmax(int v) { _datamax = v; progcnt=0; }
    double get dataProgress => progcnt > _datamax ? 1.0 : progcnt / _datamax;
    bool get isProgress => _datamax > 0;

    final ValueNotifier<int> _notify = ValueNotifier(0);
    ValueNotifier<int> get notifyInf => _notify;
    void doNotifyInf() => _notify.value++;

    int _autokey = 0;
    bool _istest = false;
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
        if (_kalive != null) {
            _kalive!.cancel();
        }
        _kalive = null;
        progmax = 0;

        _state = NetState.offline;
        developer.log('net cleared');
    }

    Future<bool> start(host, int port) {
        _istest = false;
        return _sockproc = _process(host, port);
    }

    Future<bool> startTest() {
        _istest = true;
        return _sockproc = _process('test.xdeya.ru', 65321);
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

    bool recieverOne(int cmd, void Function(BinProto) ?hnd) {
        return recieverAdd(cmd, (pro) {
            recieverDel(cmd);
            hnd!(pro);
        });
    }

    bool recieverList(int cmd, {
            void Function(BinProto) ?beg,
            void Function(BinProto) ?item,
            void Function(BinProto) ?end }) {

        return recieverOne(cmd, (pro) {
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
    
    Future<bool> requestOne(
            int cmd, String? pk, List<dynamic>? vars,
            void Function(BinProto pro) ?hnd
        ) async {
        if (_reciever.containsKey(cmd)) {
            developer.log('reciever($cmd) exists');
            return false;
        }

        if (!await autochk()) return false;
        
        bool ok =
            recieverOne(cmd, hnd) &&
            send(0x0a, 'C${pk?? ''}', [cmd, ...(vars ?? [])]);

        if (!ok) recieverDel(cmd);

        return ok;
    }
    
    Future<bool> requestList(
            int cmd, String? pk, List<dynamic>? vars,
            {
                void Function(BinProto pro) ?beg,
                void Function(BinProto pro) ?item,
                void Function(BinProto pro) ?end,
            }
        ) async {
        if (recieverContains({ cmd, cmd+1, cmd+2 })) {
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
        if (_istest) {
            _autokey    = 1;
            _autoip     = _sock!.remoteAddress;
            _autoport   = _sock!.remotePort;
        }
        else
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
}
