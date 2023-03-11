import 'package:flutter/material.dart';
import 'package:path/path.dart';
import 'dart:io';

import '../net/proc.dart';
import '../net/types.dart';
import '../pager.dart';

class PageFilesRestore extends StatelessWidget {
    static final ValueNotifier<int> _notifyList = ValueNotifier(0);
    static final List<FileItem> _flist = [];
    static void opendir(String _dir) {
        clearlist();
        final dir = Directory(_dir);
        final List<FileSystemEntity> entities = dir.listSync().toList();
        final Iterable<File> files = entities.whereType<File>();
        for (final f in files) {
            _flist.add(FileItem(name: basename(f.path), size: f.lengthSync(), path: f.path));
            _notifyList.value = _flist.length;
        }
    }
    static void clearlist() {
        _flist.clear();
        _notifyList.value = 0;
    }
    bool get _saveallow {
        for (final f in _flist) {
            if (f.checked) return true;
        }
        return false;
    }

    const PageFilesRestore({ super.key });
    
    @override
    Widget build(BuildContext context) {
        return ValueListenableBuilder(
            valueListenable: _notifyList,
            builder: (BuildContext context, count, Widget? child) {
                if (_flist.isEmpty) {
                    return const Center(
                        child: Text(
                            'Не найдено', 
                            style: TextStyle(color: Colors.black26)
                        )
                    );
                }
                
                return ListView.separated(
                    itemCount: _flist.length + 1,
                    itemBuilder: (BuildContext contextItem, int index) {
                        if (index >= _flist.length) {
                            return Padding(
                                padding: const EdgeInsets.symmetric(
                                    vertical: 8.0,
                                    horizontal: 32.0,
                                ),
                                child: Row(
                                    children: [
                                        const Spacer(),
                                        FloatingActionButton(
                                            heroTag: 'btnSave',
                                            onPressed: _saveallow ? 
                                                () {
                                                    net.saveFiles(
                                                        files: _flist.where((f) => f.checked).toList()
                                                    );
                                                    Pager.pop(context);
                                                } :
                                                null,
                                            child: const Text('Save'),
                                        ),
                                    ]
                                )
                            );
                        }

                        final f = _flist[index];
                        return Card(
                            child: ListTile(
                                trailing: const SizedBox.shrink(),
                                title: Row(children: [
                                    Checkbox(
                                        value: f.checked,
                                        onChanged: (bool? value) {
                                            f.checked = value ?? false;
                                            _notifyList.value ++;
                                        },
                                    ),
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
                );
            },
        );
    }
}
