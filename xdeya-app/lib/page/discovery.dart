import 'package:flutter/material.dart';
import '../net/wifidiscovery.dart';
import '../net/proc.dart';
import '../pager.dart';
import 'dart:developer' as developer;

class PageDiscovery extends StatelessWidget {
    const PageDiscovery({ super.key });

    @override
    Widget build(BuildContext context) {
        return ValueListenableBuilder(
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
        );
    }
}
