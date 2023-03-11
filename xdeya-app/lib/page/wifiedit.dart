import 'package:flutter/material.dart';
import 'dart:developer' as developer;

import '../net/proc.dart';
import '../pager.dart';

class PageWiFiEdit extends StatelessWidget {
    final int _index;
    final bool _ismod;
    String _ssid = '', _pass = '';
    String _orig_ssid = '', _orig_pass = '';
    final ValueNotifier<int> _chg = ValueNotifier(0);

    bool _isChanged() => (_ssid != _orig_ssid) || (_pass != _orig_pass);
    bool _isValid() => _ssid.isNotEmpty && _pass.isNotEmpty;

    PageWiFiEdit({ super.key, required int index }) : 
        _index = index,
        _ismod = (index >= 0) && (index < net.wifipass.length)
    {
        if (_ismod) {
            _ssid = net.wifipass[_index].ssid;
            _orig_ssid = _ssid;
            _pass = net.wifipass[_index].pass;
            _orig_pass = _pass;
        }
    }
    
    @override
    Widget build(BuildContext context) {
        return Padding(
            padding: const EdgeInsets.symmetric(
                vertical: 8.0,
                horizontal: 32.0,
            ),
            child: Column(
                children: [
                    Text(_ismod ? 'Изменение сети' : 'Добавление сети'),
                    Padding(
                        padding: const EdgeInsets.only(bottom: 8.0),
                        child: TextField(
                            controller: _ismod ?
                                TextEditingController(text: _ssid) :
                                null,
                            decoration: const InputDecoration(
                                labelText: 'SSID (название сети)',
                            ),
                            autofocus: _index < 0,
                            onChanged: (v) { _ssid = v; _chg.value = _chg.value + 1; },
                        ),
                    ),
                    Padding(
                        padding: const EdgeInsets.only(bottom: 8.0),
                        child: TextField(
                            controller: _ismod ?
                                TextEditingController(text: _pass) :
                                null,
                            decoration: const InputDecoration(
                                labelText: 'Пароль',
                            ),
                            autofocus: _index >= 0,
                            onChanged: (v) { _pass = v; _chg.value = _chg.value + 1; },
                        ),
                    ),
                    Row(children: [
                        _ismod ?
                            FloatingActionButton(
                                heroTag: 'btnRemove',
                                child: const Icon(Icons.close),
                                onPressed: () {
                                    developer.log('index: $_index');
                                    net.delWiFiPass(_index);
                                    Pager.pop(context);
                                }
                            ) :
                            Container(),
                        const Spacer(),
                        ValueListenableBuilder(
                            valueListenable: _chg,
                            builder: (BuildContext context, val, Widget? child) {
                                return FloatingActionButton(
                                    child: const Icon(Icons.check),
                                    onPressed: _isChanged() && _isValid() ?
                                        () {
                                            developer.log('ssid: $_ssid, pass: $_pass');
                                            if ((_ssid == '') || (_pass == '')) return;
                                            if (_ismod) {
                                                net.setWiFiPass(_index, _ssid, _pass);
                                            }
                                            else {
                                                net.addWiFiPass(_ssid, _pass);
                                            }
                                            Pager.pop(context);
                                        } :
                                        null
                                );
                            }
                        ),
                    ])
                    
                ],
            )
        );
    }
}
