import 'package:flutter/material.dart';

import '../net/proc.dart';
import '../data/files.dart';

class PageFilesBackup extends StatelessWidget {
    PageFilesBackup({ super.key });

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
                    valueListenable: files.notify,
                    builder: (BuildContext context, count, Widget? child) {
                        if (files.isEmpty) {
                            return const Center(
                                child: Text(
                                    'Не найдено', 
                                    style: TextStyle(color: Colors.black26)
                                )
                            );
                        }
                        
                        return ListView.separated(
                            itemCount: files.length,
                            itemBuilder: (BuildContext contextItem, int index) {
                                final f = files[index];
                                return Card(
                                    child: ListTile(
                                        trailing: const SizedBox.shrink(),
                                        title: Row(children: [
                                            Expanded(child: Text(f.name)),
                                            Text(
                                                f.size.toString(),
                                                style: const TextStyle(
                                                    fontSize: 10,
                                                    color: Colors.black45
                                                ),
                                                textAlign: TextAlign.right,
                                            )
                                        ])
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
