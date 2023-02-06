import 'package:flutter/material.dart';
import 'dart:developer' as developer;

import '../net/proc.dart';
import '../pager.dart';

class PageWiFiPass extends StatelessWidget {
    PageWiFiPass({ super.key });

    final ScrollController _scrollController = ScrollController();
    _scrollToBottom() {
        if (!_scrollController.hasClients) {
            return;
        }
        //_scrollController.jumpTo(_scrollController.position.maxScrollExtent);
        _scrollController.animateTo(
            _scrollController.position.maxScrollExtent,
            duration: const Duration(milliseconds: 200),
            curve: Curves.easeOut
        );
    }
    
    @override
    Widget build(BuildContext context) {
        return ValueListenableBuilder(
            valueListenable: net.notifyInf,
            builder: (BuildContext context, inf, Widget? child) {
                WidgetsBinding.instance.addPostFrameCallback((_) => _scrollToBottom());

                return ValueListenableBuilder(
                    valueListenable: net.notifyWiFiList,
                    builder: (BuildContext context, count, Widget? child) {
                        if (net.logbook.isEmpty) {
                            return const Center(
                                child: Text(
                                    'Не найдено', 
                                    style: TextStyle(color: Colors.black26)
                                )
                            );
                        }
                        
                        return ListView.separated(
                            itemCount: net.wifipass.length+1,
                            itemBuilder: (BuildContext contextItem, int index) {
                                if (index >= net.wifipass.length) {
                                    return Padding(
                                        padding: const EdgeInsets.symmetric(
                                            vertical: 8.0,
                                            horizontal: 32.0,
                                        ),
                                        child: Row(
                                            children: [
                                                const Spacer(),
                                                FloatingActionButton(
                                                    heroTag: 'btnAdd',
                                                    child: const Icon(Icons.add),
                                                    onPressed: () => Pager.push(context, PageCode.wifiedit, -1),
                                                ),
                                                const SizedBox(width: 20, height: 20),
                                                FloatingActionButton(
                                                    heroTag: 'btnSave',
                                                    child: const Text('Save'),
                                                    onPressed: () {
                                                        net.saveWiFiPass();
                                                        Pager.pop(context);
                                                    }
                                                ),
                                            ]
                                        )
                                    );
                                }

                                final wifi = net.wifipass[index];
                                return Card(
                                    child: ListTile(
                                    onTap: () {
                                        developer.log('tap on: $index');
                                        Pager.push(context, PageCode.wifiedit, index);
                                    },
                                    trailing: const SizedBox.shrink(),
                                    title: Text(wifi.ssid),
                                )
                                );
                            },
                            separatorBuilder: (BuildContext context, int index) => const Divider(),
                            controller: _scrollController,
                        );
                    },
                );
            }
        );
    }
}
