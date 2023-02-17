import 'package:flutter/material.dart';
import 'package:loading_animation_widget/loading_animation_widget.dart';
import 'pager.dart';
import 'net/wifidiscovery.dart';
import 'net/proc.dart';
import 'data/track.dart';


Widget getTitleBarDiscovery() {
    wifi.autosrch();

    return ValueListenableBuilder(
        valueListenable: wifi.notifyActive,
        builder: (BuildContext context, isActive, Widget? child) {
            return AppBar(
                title: isActive ? Row(
                    children: [
                        LoadingAnimationWidget.horizontalRotatingDots(
                            color: Colors.white,
                            size: 20,
                        ),
                        const Expanded(child: Text('Поиск устройств', textAlign: TextAlign.center))
                    ]
                ) : null,
                actions: <Widget>[
                    IconButton(
                        icon: const Icon(Icons.refresh),
                        tooltip: 'Обновить',
                        onPressed: isActive ? null : wifi.autosrch
                    ),
                ],
            );
        },
    );
}


enum MenuCode { Refresh, WiFiPass, TrackList, SaveGpx }

Widget getTitleBarClient(PageCode page) {
    return ValueListenableBuilder(
        valueListenable: net.notifyInf,
        builder: (BuildContext context, inf, Widget? child) {
            List<Widget> row = [];

            if (net.isProgress) {
                row.add(
                    SizedBox(
                        width: 150,
                        child: LinearProgressIndicator(
                                value: net.dataProgress,
                                minHeight: 10,
                                color: Colors.black54,
                        )
                    )
                );
            }
            else
            if (net.isLoading) {
                row.add(
                    LoadingAnimationWidget.horizontalRotatingDots(
                        color: Colors.white,
                        size: 20,
                    )
                );
            }

            if (net.error != null) {
                String txt;
                switch (net.error) {
                    case NetError.connect:
                        txt = 'Не могу подключиться';
                        break;

                    case NetError.disconnected:
                        txt = 'Соединение разорвано';
                        break;
                        
                    case NetError.proto:
                        txt = 'Ошибка протокола передачи';
                        break;
                        
                    case NetError.cmddup:
                        txt = 'Задублированный запрос';
                        break;
                        
                    case NetError.auth:
                        txt = 'Неверный код авторизации';
                        break;
                    
                    default:
                        txt = 'Неизвестная ошибка';
                }
                row.add(Expanded(
                    child: Text(
                        txt,
                        textAlign: TextAlign.center,
                        style: const TextStyle(color: Colors.red)
                    )
                ));
            }
            else
            if (net.state != NetState.online) {
                String ?txt;
                switch (net.state) {
                    case NetState.connecting:
                        txt = 'Подключение';
                        break;

                    case NetState.connected:
                        txt = 'Ожидание приветствия';
                        break;
                        
                    case NetState.waitauth:
                        txt = 'Авторизация на устройстве';
                        break;
                    
                    default:
                }
                if (txt != null) {
                    row.add(Expanded(
                        child: Text(
                            txt,
                            textAlign: TextAlign.center,
                        )
                    ));
                }
            }
            else
            if (Pager.title.isNotEmpty) {
                final title = Pager.title;
                row.add(Expanded(
                    child: Text(
                        title,
                        textAlign: TextAlign.center,
                    )
                ));
            }


            List<PopupMenuEntry<MenuCode>> menu = [];
            if (Pager.refreshVisible) {
                menu.add(
                    PopupMenuItem(
                        value: MenuCode.Refresh,
                        child: Row(
                            children: const [
                                Icon(Icons.refresh, color: Colors.black),
                                SizedBox(width: 8),
                                Text('Обновить'),
                            ]
                        ),
                    ),
                );
                menu.add(
                    PopupMenuDivider(),
                );
            }
            if ((Pager.top != PageCode.wifipass) && (Pager.top != PageCode.wifiedit)) {
                menu.add(
                    PopupMenuItem(
                        value: MenuCode.WiFiPass,
                        child: Row(
                            children: const [
                                Icon(Icons.wifi, color: Colors.black),
                                SizedBox(width: 8),
                                Text('WiFi-пароли'),
                            ]
                        ),
                    ),
                );
            }
            if ((Pager.top != PageCode.tracklist) && (Pager.top != PageCode.trackview)) {
                menu.add(
                    PopupMenuItem(
                        value: MenuCode.TrackList,
                        child: Row(
                            children: const [
                                Icon(Icons.polyline, color: Colors.black),
                                SizedBox(width: 8),
                                Text('Все треки'),
                            ]
                        ),
                    ),
                );
            }
            if (Pager.top == PageCode.trackview) {
                menu.add(
                    PopupMenuItem(
                        value: MenuCode.SaveGpx,
                        child: Row(
                            children: const [
                                Icon(Icons.save, color: Colors.black),
                                SizedBox(width: 8),
                                Text('Сохранить в GPX'),
                            ]
                        ),
                    ),
                );
            }

            void onSelect(MenuCode mitem) async {
                switch (mitem) {
                    case MenuCode.Refresh:
                        Pager.refreshPressed!();
                        break;
                    case MenuCode.WiFiPass:
                        net.requestWiFiPass();
                        Pager.push(context, PageCode.wifipass);
                        break;
                    case MenuCode.TrackList:
                        Pager.push(context, PageCode.tracklist);
                        break;
                    case MenuCode.SaveGpx:
                        final dt = net.trkinfo.dtBeg.replaceAll(' ', '_').replaceAll(':', '.');
                        trackSaveGpx(
                            filename: 'track_${dt}_jmp-${net.trkinfo.jmpnum}.gpx',
                            data: net.trkGPX
                        );
                        break;
                }
            };
            
            return AppBar(
                leading: Navigator.canPop(context) ?
                    IconButton(
                        icon: const Icon(Icons.navigate_before),
                        onPressed: () => Pager.pop(context),
                    ) : null,
                title: Row(children: row),
                actions:  [
                    PopupMenuButton<MenuCode>(
                        tooltip: "Меню",
                        onSelected: onSelect,  
                        itemBuilder: (context) => menu
                    ),
                ],
            );
        }
    );
}
