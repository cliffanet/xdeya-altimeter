import 'package:flutter/material.dart';
import 'package:masked_text/masked_text.dart';
import 'dart:developer' as developer;

import '../data/logbook.dart';
import '../net/proc.dart';
import '../pager.dart';

Widget _pageAuth() {
    return Container(
        //color: Colors.black54,
        child: Padding(
            padding: const EdgeInsets.symmetric(
                vertical: 8.0,
                horizontal: 32.0,
            ),
            child: Column(
                children: [
                    Padding(
                        padding: const EdgeInsets.only(bottom: 8.0),
                        child: MaskedTextField(
                            mask: '####',
                            maskFilter: { "#": RegExp(r'[0-9A-Fa-f]') },
                            onChanged: (v) {
                                if (v.length != 4) {
                                    return;
                                }
                                net.requestAuth( v );
                            },
                            decoration: const InputDecoration(
                                labelText: 'Введите код с экрана',
                            ),
                            //autofocus: true,
                        ),
                    )
                ],
            )
        )
    );
}

class PageLogBook extends StatelessWidget {
    PageLogBook({ super.key });

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
                if (net.state == NetState.waitauth) {
                    return _pageAuth();
                }

                return ValueListenableBuilder(
                    valueListenable: logbook.notify,
                    builder: (BuildContext context, count, Widget? child) {
                        if (logbook.isEmpty) {
                            return const Center(
                                child: Text(
                                    'Не найдено', 
                                    style: TextStyle(color: Colors.black26)
                                )
                            );
                        }

                        WidgetsBinding.instance.addPostFrameCallback((_) => _scrollToBottom());
                        
                        return ListView.separated(
                            itemCount: logbook.length,
                            itemBuilder: (BuildContext contextItem, int index) {
                                final lb = logbook[index];
                                return Card(
                                    child: ListTile(
                                        onTap: () {
                                            developer.log('tap on: $index');
                                            Pager.push(context, PageCode.jumpinfo, index);
                                        },
                                        trailing: const SizedBox.shrink(),
                                        title: Row(children: [
                                            Text(lb.num.toString()),
                                            const Spacer(),
                                            Text(lb.dtBeg)
                                        ]),
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
