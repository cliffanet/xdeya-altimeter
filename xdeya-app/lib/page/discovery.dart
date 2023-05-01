import 'package:flutter/material.dart';
import 'package:file_picker/file_picker.dart';
import '../net/wifidiscovery.dart';
import '../net/proc.dart';
import '../data/files.dart';
import '../pager.dart';
import 'dart:developer' as developer;

class PageDiscovery extends StatelessWidget {
    const PageDiscovery({ super.key });

    @override
    Widget build(BuildContext context) {
        return Column(children: [
            Expanded(child:
                ValueListenableBuilder(
                    valueListenable: wifi.notifyDevice,
                    builder: (BuildContext context, count, Widget? child) {
                        if (wifi.device.isEmpty) {
                            return const Center(
                                child: Text(
                                    'Не найдено', 
                                    style: TextStyle(color: Colors.black26)
                                )
                            );
                        }
                    
                        return ListView.separated(
                            itemCount: wifi.device.length,
                            itemBuilder: (BuildContext contextItem, int index) {
                                final w = wifi.device[index];
                                return Card(
                                    child: ListTile(
                                        onTap: () {
                                            developer.log('tap on: $index');
                                            net.start(w.ip, w.port);
                                            Pager.push(context, PageCode.logbook);
                                        },
                                        trailing: const SizedBox.shrink(),
                                        title: Text(w.name),
                                    )
                                );
                            },
                            separatorBuilder: (BuildContext context, int index) => const Divider(),
                        );
                    },
                ),
            ),

            ListTile(
                onTap: () async {
                    developer.log('tap on dir ref');
                    String? dir = await FilePicker.platform.getDirectoryPath();
                    if (dir == null) {
                        return;
                    }
                    filesLoadJumpTrack(dir);
                    Pager.push(context, PageCode.logbook);
                },
                trailing: const SizedBox.shrink(),
                title: const Text("открыть папку с файлами", textAlign: TextAlign.center),
            ),

            ListTile(
                onTap: () {
                    developer.log('tap on test ref');
                    net.startTest();
                    Pager.push(context, PageCode.logbook);
                },
                trailing: const SizedBox.shrink(),
                title: const Text("тестовый вход", textAlign: TextAlign.center),
            )
        ]);
    }
}
