import 'package:flutter/foundation.dart';
//import 'dart:typed_data';
import 'dart:io';
import 'package:udp/udp.dart';
import 'dart:developer' as developer;

final wifi = WiFiDiscovery();

class WiFiDevice {
    final String name;
    final InternetAddress ip;
    final int port;
    const WiFiDevice({ required this.name, required this.ip, required this.port });

    bool operator ==(covariant WiFiDevice other) {
        return 
            (other.name == name) &&
            (other.ip == ip) &&
            (other.port == port);
    }
}

class WiFiDiscovery {
    final ValueNotifier<bool> _active = ValueNotifier(false);
    bool get isActive => _active.value;
    ValueNotifier<bool> get notifyActive => _active;

    List<WiFiDevice> _device = [];
    List<WiFiDevice> get device => _device;
    ValueNotifier<int> _devcnt = ValueNotifier(0);
    int get count => _devcnt.value;
    ValueNotifier<int> get notifyDevice => _devcnt;

    void clear() {
        _device.clear();
        _devcnt.value = 0;
    }

    Future<bool> search({ void Function(bool) ?onState, void Function() ?onChange }) async {
        if (_active.value) {
            developer.log('search already active, fail new session');
            return false;
        }
        developer.log('search start');
        _active.value = true;
        if (onState != null) onState(true);
        UDP rcv = await UDP.bind(Endpoint.any(port: const Port(3310)));
        if (rcv.socket == null) {
            _active.value = false;
            if (onState != null) onState(false);
            return false;
        }
        developer.log('search binded');

        // временный список, пока ищём,
        // этим списком заменим рабочий _device
        // по окончанию поиска
        List<WiFiDevice> newdevice = [];

        rcv.asStream().listen((e) {
            if ((e == null) || (e.data.length <= 2)) {
                return;
            }

            final int port = e.data.buffer.asByteData().getUint16(0, Endian.big);
            final name = String.fromCharCodes(e.data.sublist(2));
            final w = WiFiDevice(name: name, ip: e.address, port: port);
            developer.log('recv: $name (${e.address}:$port)');

            // временный back-список
            bool found = false;
            for (var d in newdevice) {
                if (d != w) continue;
                found = true;
                break;
            }
            if (!found) {
                newdevice.add(w);
            }

            // Добавляем сразу в два списка,
            // чтобы пока идёт поиск, новые устройства
            // были во front-списке
            found = false;
            for (var d in _device) {
                if (d != w) continue;
                found = true;
                break;
            }
            if (!found) {
                _device.add(w);
                _devcnt.value = _device.length;
                if (onChange != null) onChange();
            }
        });

        await Future.delayed(const Duration(seconds:20));
        
        developer.log('search finish');

        rcv.close();
        _device = newdevice;
        _devcnt.value = _device.length;
        if (onChange != null) onChange();
        _active.value = false;
        if (onState != null) onState(false);

        return true;
    }
}
