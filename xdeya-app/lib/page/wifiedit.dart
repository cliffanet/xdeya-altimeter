import 'package:flutter/material.dart';
import 'package:masked_text/masked_text.dart';
import 'dart:developer' as developer;

import '../net/proc.dart';
import '../pager.dart';

class PageWiFiEdit extends StatelessWidget {
    final int _index;
    final bool _ismod;
    String _ssid = '', _pass = '';

    PageWiFiEdit({ super.key, required int index }) : 
        _index = index,
        _ismod = (index >= 0) && (index < net.wifipass.length)
    {
        if (_ismod) {
            _ssid = net.wifipass[_index].ssid;
            _pass = net.wifipass[_index].pass;
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
                            onChanged: (v) => _ssid = v,
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
                            onChanged: (v) => _pass = v,
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
                        FloatingActionButton(
                            child: const Icon(Icons.check),
                            onPressed: () {
                                developer.log('ssid: $_ssid, pass: $_pass');
                                if ((_ssid == '') || (_pass == '')) return;
                                if (_ismod) {
                                    net.setWiFiPass(_index, _ssid, _pass);
                                }
                                else {
                                    net.addWiFiPass(_ssid, _pass);
                                }
                                Pager.pop(context);
                            }
                        ),
                    ])
                    
                ],
            )
        );
    }
}
