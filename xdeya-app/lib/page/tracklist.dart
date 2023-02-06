import 'package:flutter/material.dart';
import 'dart:developer' as developer;

import '../net/proc.dart';
import '../pager.dart';

class PageTrackList extends StatelessWidget {
    PageTrackList({ super.key });

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
                    valueListenable: net.notifyTrkList,
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
                            itemCount: net.trklist.length,
                            itemBuilder: (BuildContext contextItem, int index) {
                                final trk = net.trklist[net.trklist.length - index - 1];
                                return Card(
                                    child: ListTile(
                                    onTap: () {
                                        developer.log('tap on: $index');
                                        net.requestTrkData(trk);
                                        Pager.push(context, PageCode.trackview);
                                    },
                                    trailing: const SizedBox.shrink(),
                                    title: Text('трэк: ${trk.dtBeg}'),
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
