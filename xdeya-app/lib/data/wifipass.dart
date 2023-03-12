
import 'dart:collection';
import 'package:flutter/foundation.dart';
import 'dart:developer' as developer;

import '../net/proc.dart';

/*//////////////////////////////////////
 *
 *  WiFiPass
 * 
 *//////////////////////////////////////
class WiFiPass {
    final String ssid;
    final String pass;
    final String _orig_ssid;
    final String _orig_pass;

    WiFiPass({
        required this.ssid,
        required this.pass,
    }) : _orig_ssid = '', _orig_pass = '';

    WiFiPass.byvars(List<dynamic> vars) :
        ssid        = (vars.isNotEmpty) && (vars[0]) is String ? vars[0] : '',
        _orig_ssid  = (vars.isNotEmpty) && (vars[0]) is String ? vars[0] : '',
        pass        =  (vars.length > 1) && (vars[1]) is String ? vars[1] : '',
        _orig_pass  =  (vars.length > 1) && (vars[1]) is String ? vars[1] : '';
    
    WiFiPass.edited(this.ssid, this.pass, WiFiPass orig) :
        _orig_ssid  = orig._orig_ssid,
        _orig_pass  = orig._orig_pass;
    
    bool get isChanged {
        return
            (_orig_ssid != ssid) ||
            (_orig_pass != pass);
    }
    bool get isValid => ssid.isNotEmpty;
}

/*//////////////////////////////////////
 *
 *  WiFiPass
 * 
 *//////////////////////////////////////
final wifipass = DataWiFiPass();
class DataWiFiPass extends ListBase<WiFiPass> {
    final List<WiFiPass> _list;
    DataWiFiPass() : _list = [];

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
    WiFiPass operator [](int index) => _list[index];
    @override
    void operator []=(int index, WiFiPass value) {
        _list[index]=value;
        _sz.value = 0;
        _sz.value = _list.length;
    }

    Future<bool> netRequest({ Function() ?onLoad }) async {
        return net.requestList(
            0x37, null, null,
            beg: (pro) {
                List<dynamic> ?v = pro.rcvData('N');
                if ((v == null) || v.isEmpty) {
                    return;
                }

                developer.log('wifipass beg ${v[0]}');
                _list.clear();
                _sz.value = 0;
                net.progmax = v[0];
            },
            item: (pro) {
                List<dynamic> ?v = pro.rcvData('ssN');
                if ((v == null) || v.isEmpty) {
                    return;
                }
                _list.add(WiFiPass.byvars(v));
                _sz.value = _list.length;
                net.progcnt = (v.length > 2) && (v[2]) is int ? v[2] : 0;
            },
            end: (pro) {
                pro.rcvNext();
                developer.log('wifipass end');
                net.progmax = 0;
                if (onLoad != null) onLoad();
            }
        );
    }

    void push(String ssid, String pass) {
        _list.add(WiFiPass(ssid: ssid, pass: pass));
        _sz.value = _list.length;
    }
    void set(int index, String ssid, String pass) {
        if ((index < 0) || (index >= _list.length)) {
            return;
        }
        _list[index] = WiFiPass.edited(ssid, pass, _list[index]);
        _sz.value = 0;
        _sz.value = _list.length;
    }
    void del(int index) {
        if ((index < 0) || (index >= _list.length)) {
            return;
        }
        _list.removeAt(index);
        _sz.value = _list.length;
    }
    bool get isChanged {
        for (final w in _list) {
            if (w.isChanged) return true;
        }
        return false;
    }

    Future<bool> netSave({ Function() ?onDone }) async {
        if (!await net.autochk()) return false;

        // это просто заглушка (команда 0x4a временно отправляется по завершению приёма)
        if (!net.recieverAdd(0x4a, (pro) { pro.rcvNext(); })) {
            return false;
        }
        bool ok = net.confirmerAdd(0x41, (err) {
            net.recieverDel(0x4a);
            if ((err == null) && (onDone != null)) onDone();
        });
        if (!ok) {
            net.recieverDel(0x4a);
            return false;
        }

        if (!net.send(0x41)) {
            net.recieverDel(0x4a);
            net.confirmerDel(0x41);
            return false;
        }
        net.doNotifyInf();

        for (var wifi in _list) {
            if (!net.send(0x42, 'ss', [wifi.ssid, wifi.pass])) {
                net.recieverDel(0x4a);
                net.confirmerDel(0x41);
                return false;
            }
        }

        if (!net.send(0x43)) {
            net.recieverDel(0x4a);
            net.confirmerDel(0x41);
            return false;
        }

        return true;
    }
}
