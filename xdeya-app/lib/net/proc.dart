import 'dart:ffi';

import 'package:flutter/foundation.dart';
import 'dart:io';
import 'dart:async';
import 'dart:developer' as developer;

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
    Future<bool> _autochk() async {
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
        if (!recieverAdd(0x02, () {
                recieverDel(0x02);
                // 0x03 (authreply) вы добавляем, если у нас есть
                // autokey с предыдущей авторизации, но он может не пройти
                recieverDel(0x03);
                _pro.rcvNext();
                _state = NetState.waitauth;
                _autokey = 0;
                _autoip = null;
                _autoport = null;
                doNotifyInf();
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
                Function() ?hnd = _reciever[ _pro.rcvCmd ];

                if (hnd != null) {
                    hnd();
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
                !(v[0] is int) || !(v[1] is int)
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

    final Map<int, void Function()> _reciever = {};
    bool get isLoading => _reciever.isNotEmpty || _confirmer.isNotEmpty;

    bool recieverAdd(int cmd, void Function() hnd) {
        if (_sock == null)  {
            _err = NetError.disconnected;
            doNotifyInf();
            return false;
        }

        if (_reciever[cmd] != null) {
            return false;
        }

        _reciever[cmd] = hnd;

        return true;
    }
    
    bool recieverDel(int cmd) {
        if (_reciever[cmd] == null) {
            return false;
        }

        _reciever.remove(cmd);

        return true;
    }

    void Function()? reciever(int cmd) {
        return _reciever[cmd];
    }

    /*//////////////////////////////////////
     *
     *  confirmer
     * 
     *//////////////////////////////////////

    final Map<int, void Function(int ?err)> _confirmer = {};

    bool confirmerAdd(int cmd, void Function(int ?err) hnd) {
        if (_confirmer[cmd] != null) {
            return false;
        }

        _confirmer[cmd] = hnd;

        return true;
    }
    
    bool confirmerDel(int cmd) {
        if (_confirmer[cmd] == null) {
            return false;
        }

        _confirmer.remove(cmd);

        return true;
    }

    void Function(int ?err)? confirmer(int cmd) {
        return _confirmer[cmd];
    }


    /*//////////////////////////////////////
     *
     *  Auth
     * 
     *//////////////////////////////////////

    bool _auth_recv() {
        recieverDel(0x02);
        recieverDel(0x03);
        List<dynamic> ?v = _pro.rcvData('Cn');
        if ((v == null) || v.isEmpty || (v[0] > 0)) {
            _errstop(NetError.auth);
            return false;
        }
        if ((v != null) && (v.length >= 2) && (v[1] > 0) && (_sock != null)) {
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
        doNotifyInf();

        net.requestLogBookDefault();
        
        return true;
    }
    bool requestAuth(String codehex, { Function() ?onReplyOk, Function() ?onReplyErr }) {
        int code = int.parse(codehex, radix: 16);
        if (code == 0) {
            return false;
        }

        bool ok = recieverAdd(0x03, () {
                final on = _auth_recv() ? onReplyOk : onReplyErr;
                if (on != null) on();
            });
        if (!ok) return false;
        doNotifyInf();

        return send(0x03, 'n', [code]);
    }

    /*//////////////////////////////////////
     *
     *  LogBook
     * 
     *//////////////////////////////////////

    final ValueNotifier<int> _logbooksz = ValueNotifier(0);
    ValueNotifier<int> get notifyLogBook => _logbooksz;
    final List<LogBook> _logbook = [];
    List<LogBook> get logbook => _logbook;

    Future<bool> requestLogBook({ int beg = 50, int count = 50, Function() ?onLoad }) async {
        if (!await _autochk()) return false;

        if (_rcvelm.contains(NetRecvElem.logbook)) {
            return false;
        }
        bool ok = recieverAdd(0x31, () {
                recieverDel(0x31);
                List<dynamic> ?v = _pro.rcvData('NN');
                if ((v == null) || v.isEmpty) {
                    return;
                }

                developer.log('logbook beg ${v[0]}, ${v[1]}');
                _datamax = v[0] < v[1] ? v[0] : v[1];
                _logbook.clear();
                _logbooksz.value = 0;
                _datacnt = 0;
                doNotifyInf();

                recieverAdd(0x32, () {
                    List<dynamic> ?v = _pro.rcvData(LogBook.pk);
                    if ((v == null) || v.isEmpty) {
                        return;
                    }
                    _logbook.add(LogBook.byvars(v));
                    _logbooksz.value = _logbook.length;
                    _datacnt = _logbook.length;
                    doNotifyInf();
                });
                recieverAdd(0x33, () {
                    recieverDel(0x32);
                    recieverDel(0x33);
                    developer.log('logbook end $_datacnt / $_datamax');
                    _pro.rcvNext();
                    _rcvelm.remove(NetRecvElem.logbook);
                    _datamax = 0;
                    _datacnt = 0;
                    doNotifyInf();
                    if (onLoad != null) onLoad();
                });
            });
        if (!ok) return false;
        if (!send(0x31, 'NN', [beg, count])) {
            recieverDel(0x31);
            return false;
        }
        _rcvelm.add(NetRecvElem.logbook);
        doNotifyInf();

        return true;
    }

    Future<bool> requestLogBookDefault() {
        return requestLogBook(
            onLoad: () => requestTrkList()
        );
    }

    /*//////////////////////////////////////
     *
     *  TrkList
     * 
     *//////////////////////////////////////

    final ValueNotifier<int> _trklistsz = ValueNotifier(0);
    ValueNotifier<int> get notifyTrkList => _trklistsz;
    final List<TrkItem> _trklist = [];
    List<TrkItem> get trklist => _trklist;
    List<TrkItem> trkListByJmp(LogBook jmp) {
        return _trklist.where((trk) => trk.jmpnum == jmp.num).toList();
    }

    Future<bool> requestTrkList({ Function() ?onLoad }) async {
        if (!await _autochk()) return false;

        if (_rcvelm.contains(NetRecvElem.tracklist)) {
            return false;
        }
        bool ok = recieverAdd(0x51, () {
                recieverDel(0x51);
                _pro.rcvNext();

                developer.log('trklist beg');
                _trklist.clear();
                _trklistsz.value = 0;
                doNotifyInf();

                recieverAdd(0x52, () {
                    List<dynamic> ?v = _pro.rcvData('NNNNTNC');
                    if ((v == null) || v.isEmpty) {
                        return;
                    }
                    _trklist.add(TrkItem.byvars(v));
                    _trklistsz.value = _trklist.length;
                    doNotifyInf();
                });
                recieverAdd(0x53, () {
                    recieverDel(0x52);
                    recieverDel(0x53);
                    developer.log('trklist end');
                    _pro.rcvNext();
                    _rcvelm.remove(NetRecvElem.tracklist);
                    doNotifyInf();
                    if (onLoad != null) onLoad();
                });
            });
        if (!ok) return false;
        if (!send(0x51)) {
            recieverDel(0x51);
            return false;
        }
        _rcvelm.add(NetRecvElem.tracklist);
        doNotifyInf();

        return true;
    }

    /*//////////////////////////////////////
     *
     *  TrkData
     * 
     *//////////////////////////////////////

    // trkinfo
    TrkInfo _trkinfo = TrkInfo.byvars([]);
    TrkInfo get trkinfo => _trkinfo;
    // trkdata
    final ValueNotifier<int> _trkdatasz = ValueNotifier(0);
    ValueNotifier<int> get notifyTrkData => _trkdatasz;
    final List<Struct> _trkdata = [];
    List<Struct> get trkdata => _trkdata;
    bool trkSatValid(int i) => ((_trkdata[i]['flags'] ?? 0) & 0x0001) > 0;
    // trkCenter
    final ValueNotifier<Struct ?> _trkcenter = ValueNotifier(null);
    Struct ? get trkCenter => _trkcenter.value;
    ValueNotifier<Struct ?> get notifyTrkCenter => _trkcenter;

    Future<bool> Function() ?_reloadTrkData;
    Future<bool> reloadTrkData() async {
        return
            _reloadTrkData != null ?
                await _reloadTrkData!() :
                false;
    }
    Future<bool> requestTrkData(TrkItem trk, { Function() ?onLoad, Function(Struct) ?onCenter }) async {
        if (!await _autochk()) return false;

        if (_rcvelm.contains(NetRecvElem.trackdata)) {
            _reloadTrkData = null;
            return false;
        }
        _reloadTrkData = () => requestTrkData(trk, onLoad:onLoad, onCenter:onCenter);

        bool ok = recieverAdd(0x54, () {
                recieverDel(0x54);
                List<dynamic> ?v = _pro.rcvData('NNNNTNH');
                if ((v == null) || v.isEmpty) {
                    return;
                }
                _trkinfo = TrkInfo.byvars(v);

                developer.log('trkdata beg ${_trkinfo.jmpnum}, ${_trkinfo.dtBeg}');
                _datamax = (_trkinfo.fsize-32) ~/ 64;
                _trkdata.clear();
                _trkdatasz.value = 0;
                _trkcenter.value = null;
                _datacnt = 0;
                doNotifyInf();

                recieverAdd(0x55, () {
                    List<dynamic> ?v = _pro.rcvData(pkLogItem);
                    if ((v == null) || v.isEmpty) {
                        return;
                    }
                    Struct ti = fldUnpack(fldLogItem, v);
                    _trkdata.add(ti);
                    _trkdatasz.value = _trkdata.length;
                    _datacnt = _trkdata.length;
                    if ((_trkcenter.value == null) && (((ti['flags'] ?? 0) & 0x0001) > 0)) {
                        _trkcenter.value = ti;
                        if (onCenter != null) onCenter(ti);
                    }
                    doNotifyInf();
                });
                recieverAdd(0x56, () {
                    recieverDel(0x55);
                    recieverDel(0x56);
                    developer.log('trkdata end $_datacnt / $_datamax');
                    _pro.rcvNext();
                    _rcvelm.remove(NetRecvElem.trackdata);
                    _datamax = 0;
                    _datacnt = 0;
                    doNotifyInf();
                    if (onLoad != null) onLoad();
                });
            });
        if (!ok) return false;
        if (!send(0x54, 'NNNTC', [trk.id, trk.jmpnum, trk.jmpkey, trk.tmbeg, trk.fnum])) {
            recieverDel(0x54);
            return false;
        }
        _rcvelm.add(NetRecvElem.trackdata);
        doNotifyInf();

        return true;
    }

    String get trkGeoJson {
        String features = '';
        int i=0;
        int segid = 0;
        while (i < _trkdata.length) {
            while ( // пропустим все точки без спутников
                    (i < _trkdata.length) &&
                    !trkSatValid(i)
                ) {
                i++;
            }
            if (i >= _trkdata.length) {
                break;
            }

            Struct p = _trkdata[i];
            String pstate = p['state'];

            double lat = p['lat'] / 10000000;
            double lon = p['lon'] / 10000000;
            String crd = '[$lat,$lon]';

            int n = 1;
            i++;
            while (
                    (i < _trkdata.length) &&
                    (n < 5) &&
                    (_trkdata[i]['state'] == pstate) &&
                    trkSatValid(i)
                ) {
                double lat = _trkdata[i]['lat'] / 10000000;
                double lon = _trkdata[i]['lon'] / 10000000;
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

    String get trkGPX {
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

        while (i < _trkdata.length) {
            if ((seg > 0) && (
                    !trkSatValid(i) ||
                    (pstate != _trkdata[i]['state'])
                )) {
                data.add('</trkseg>');
                seg = 0;
            }
            while ( // пропустим все точки без спутников
                    (i < _trkdata.length) &&
                    !trkSatValid(i)
                ) {
                i++;
            }
            if (i >= _trkdata.length) {
                break;
            }

            if (seg == 0) {
                data.add('<trkseg>');
            }

            Struct p = _trkdata[i];
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

    /*//////////////////////////////////////
     *
     *  WiFi Pass
     * 
     *//////////////////////////////////////

    final ValueNotifier<int> _wifipasssz = ValueNotifier(0);
    ValueNotifier<int> get notifyWiFiList => _wifipasssz;
    final List<WiFiPass> _wifipass = [];
    List<WiFiPass> get wifipass => _wifipass;

   Future<bool> requestWiFiPass({ Function() ?onLoad }) async {
        if (!await _autochk()) return false;

        if (_rcvelm.contains(NetRecvElem.wifipass)) {
            return false;
        }
        bool ok = recieverAdd(0x37, () {
                recieverDel(0x37);
                List<dynamic> ?v = _pro.rcvData('N');
                if ((v == null) || v.isEmpty) {
                    return;
                }

                developer.log('wifipass beg ${v[0]}');
                _datamax = v[0];
                _wifipass.clear();
                _wifipasssz.value = 0;
                _datacnt = 0;
                doNotifyInf();

                recieverAdd(0x38, () {
                    List<dynamic> ?v = _pro.rcvData('ssN');
                    if ((v == null) || v.isEmpty) {
                        return;
                    }
                    _wifipass.add(WiFiPass.byvars(v));
                    _wifipasssz.value = _wifipass.length;
                    _datacnt = (v.length > 2) && (v[2]) is int ? v[2] : 0;
                    doNotifyInf();
                });
                recieverAdd(0x39, () {
                    recieverDel(0x38);
                    recieverDel(0x39);
                    developer.log('wifipass end $_datacnt / $_datamax');
                    _pro.rcvNext();
                    _rcvelm.remove(NetRecvElem.wifipass);
                    _datamax = 0;
                    _datacnt = 0;
                    doNotifyInf();
                    if (onLoad != null) onLoad();
                });
            });
        if (!ok) return false;
        if (!send(0x37)) {
            recieverDel(0x37);
            return false;
        }
        _rcvelm.add(NetRecvElem.wifipass);
        doNotifyInf();

        return true;
    }

    void addWiFiPass(String ssid, String pass) {
        _wifipass.add(WiFiPass(ssid: ssid, pass: pass));
        _wifipasssz.value = _wifipass.length;
    }
    void setWiFiPass(int index, String ssid, String pass) {
        if ((index < 0) || (index >= _wifipass.length)) {
            return;
        }
        _wifipass[index] = WiFiPass.edited(ssid, pass, _wifipass[index]);
        _wifipasssz.value = 0;
        _wifipasssz.value = _wifipass.length;
    }
    void delWiFiPass(int index) {
        if ((index < 0) || (index >= _wifipass.length)) {
            return;
        }
        _wifipass.removeAt(index);
        _wifipasssz.value = _wifipass.length;
    }
    bool get wifiChanged {
        for (final w in _wifipass) {
            if (w.isChanged) return true;
        }
        return false;
    }

    Future<bool> saveWiFiPass({ Function() ?onDone }) async {
        if (!await _autochk()) return false;

        if (_rcvelm.contains(NetRecvElem.wifisave)) {
            return false;
        }
        // это просто заглушка (команда 0x4a временно отправляется по завершению приёма)
        if (!recieverAdd(0x4a, () { _pro.rcvNext(); })) {
            return false;
        }
        bool ok = confirmerAdd(0x41, (err) {
            recieverDel(0x4a);
            _rcvelm.remove(NetRecvElem.wifisave);
            doNotifyInf();
            if ((err == null) && (onDone != null)) onDone();
        });
        if (!ok) {
            recieverDel(0x4a);
            return false;
        }

        if (!send(0x41)) {
            recieverDel(0x4a);
            confirmerDel(0x41);
            return false;
        }
        _rcvelm.add(NetRecvElem.wifisave);
        doNotifyInf();

        for (var wifi in _wifipass) {
            if (!send(0x42, 'ss', [wifi.ssid, wifi.pass])) {
                recieverDel(0x4a);
                confirmerDel(0x41);
                return false;
            }
        }

        if (!send(0x43)) {
            recieverDel(0x4a);
            confirmerDel(0x41);
            return false;
        }

        return true;
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
        if (!await _autochk()) return false;

        if (_rcvelm.contains(NetRecvElem.files)) {
            return false;
        }
        _filesClear();

        bool ok = recieverAdd(0x5a, () {
                recieverDel(0x5a);
                List<dynamic> ?v = _pro.rcvData('nN');
                if ((v == null) || v.isEmpty) {
                    return;
                }
                _filesClear();
                _datamax = v[1];
                _datacnt = 0;
                doNotifyInf();

                void recvfin() {
                    recieverDel(0x5b);
                    recieverDel(0x5e);
                    _pro.rcvNext();
                    developer.log('requestFiles finish');
                    _rcvelm.remove(NetRecvElem.files);
                    if (onLoad != null) onLoad();
                    _datamax = 0;
                    _datacnt = 0;
                    doNotifyInf();
                }
                recieverAdd(0x5e, recvfin);

                developer.log('requestFiles beg proc ${v[0]} / ${v[1]} bytes');
                void recvfile() async {
                    recieverDel(0x5b);
                    recieverDel(0x5e);
                    List<dynamic> ?v = _pro.rcvData('Ns');
                    if ((v == null) || (v.length < 2)) {
                        return;
                    }
                    developer.log('requestFiles beg file: ${v[1]} (${v[0]} bytes)');
                    files.add(FileItem.byvars(v));
                    notifyFilesList.value = files.length;
                    doNotifyInf();

                    final file = File(dir + '/' + v[1]);
                    if (file.existsSync()) {
                        file.deleteSync();
                    }
                    final fh = file.openSync(mode: FileMode.writeOnly);

                    recieverAdd(0x5c, () {
                        List<dynamic> ?v = _pro.rcvData('B');
                        if ((v == null) || v.isEmpty || (v[0] is! Uint8List)) {
                            return;
                        }
                        final data = v[0] as Uint8List;
                        developer.log('requestFiles data size=${data.length}');
                        _datacnt += data.length;
                        fh.writeFromSync(data.toList());
                        doNotifyInf();
                    });
                    recieverAdd(0x5d, () {
                        recieverDel(0x5c);
                        recieverDel(0x5d);
                        _pro.rcvNext();
                        fh.close();
                        developer.log('requestFiles end file');
                        recieverAdd(0x5b, recvfile);
                        recieverAdd(0x5e, recvfin);
                        doNotifyInf();
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
        if (!await _autochk()) return false;

        if (_rcvelm.contains(NetRecvElem.filessave)) {
            return false;
        }
        if (files.isEmpty) {
            return false;
        }
        bool ok = confirmerAdd(0x5a, (err) {
            _rcvelm.remove(NetRecvElem.filessave);
            doNotifyInf();
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
