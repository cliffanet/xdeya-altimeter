import 'package:flutter/material.dart';
import 'dart:developer' as developer;
import '../data/logbook.dart';
import '../data/trkdata.dart';
import '../data/trklist.dart';
import '../net/proc.dart';
import 'tracklist.dart';
import '../pager.dart';


class _LWitem {
    final String name;
    final String val;
    final TrkItem ?trk;
    _LWitem({ required this.name, required this.val, this.trk });
}

class PageJumpInfo extends StatelessWidget {
    final ValueNotifier<int> _index;

    PageJumpInfo({ super.key, required int index }) : _index = ValueNotifier(index);
    
    @override
    Widget build(BuildContext context) {
        return ValueListenableBuilder(
            valueListenable: _index,
            builder: (BuildContext context, ind, Widget? child) {
                if ((_index.value < 0) || (_index.value >= logbook.length)) {
                    return const Center(
                            child: Text(
                                'Нет информации', 
                                style: TextStyle(color: Colors.black26)
                            )
                        );
                }

                final LogBook jmp = logbook[_index.value];
                final List<_LWitem> lw = [
                    _LWitem(
                        name:   'Длительность взлёта:',
                        val:    jmp.timeTakeoff
                    ),
                    _LWitem(
                        name:   'Время отделения:',
                        val:    jmp.dtBeg
                    ),
                    _LWitem(
                        name:   'Высота отделения:',
                        val:    '${jmp.altBeg.toString()} m'
                    ),
                    _LWitem(
                        name:   'Время падения:',
                        val:    jmp.timeFF
                    ),
                    _LWitem(
                        name:   'Высота открытия:',
                        val:    '${jmp.altCnp.toString()} m'
                    ),
                    _LWitem(
                        name:   'Время пилотирования:',
                        val:    jmp.timeCnp
                    )
                ];
                final ibeg = lw.length;

                List<TrkItem> trkl = trklist.byJmp(jmp);
                for (TrkItem trk in trkl) {
                    lw.add(_LWitem(name: '', val: '', trk: trk));
                }
                
                return ListView.separated(
                    itemCount: lw.length+1,
                    itemBuilder: (BuildContext contextItem, int index) {
                        if (index == 0) {
                            final List<Widget> row = [];

                            if (_index.value > 0) {
                                final LogBook jmp = logbook[_index.value-1];
                                row.add(
                                    IconButton(
                                        tooltip: jmp.num.toString(),
                                        icon: const Icon(Icons.arrow_circle_left),
                                        onPressed: () => _index.value--,
                                    )
                                );
                            }
                            else {
                                row.add(const SizedBox(width: 40, height: 40));
                            }

                            row.add(
                                Expanded(
                                    child: Text(
                                        '№ ${jmp.num}',
                                        textAlign: TextAlign.center,
                                        style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                                    )
                                )
                            );

                            if (_index.value < (logbook.length-1)) {
                                final LogBook jmp = logbook[_index.value+1];
                                row.add(
                                    IconButton(
                                        tooltip: jmp.num.toString(),
                                        icon: const Icon(Icons.arrow_circle_right),
                                        onPressed: () => _index.value++,
                                    )
                                );
                            }
                            else {
                                row.add(const SizedBox(width: 40, height: 40));
                            }

                            return Row( children: row );
                        }
                        final li = lw[index-1];
                        
                        bool loadTrk() {
                            if (li.trk == null) {
                                return false;
                            }
                            if (net.isUsed) {
                                trk.netRequest(li.trk ?? TrkItem.byvars([]));
                            }
                            else
                            if (li.trk!.file != '') {
                                trk.loadFile(file: li.trk!.file);
                            }

                            return true;
                        }

                        final btn = <Widget>[ Card(
                            child: li.trk == null ?
                                Row(
                                    crossAxisAlignment: CrossAxisAlignment.center,
                                    children: [
                                        Padding(
                                            padding: const EdgeInsets.all(8.0),
                                            child: Text(
                                                li.name,
                                                maxLines: 1,
                                                softWrap: false,
                                                overflow: TextOverflow.fade,
                                            )
                                        ),
                                        Flexible(
                                            fit: FlexFit.tight,
                                            child: Padding(
                                                padding: const EdgeInsets.all(8.0),
                                                child: Text(
                                                    li.val,
                                                    maxLines: 1,
                                                    softWrap: false,
                                                    overflow: TextOverflow.fade,
                                                    textAlign: TextAlign.right,
                                                )
                                            ),
                                        ),
                                    ],
                                ) :
                                ListTile(
                                    onTap: () {
                                        developer.log('trackview on: $index');
                                        if (!loadTrk()) return;
                                        Pager.push(context, PageCode.trackcube);
                                    },
                                    trailing: const SizedBox.shrink(),
                                    title: TrackTile(li.trk!),
                                )
                        ) ];

                        if (li.trk != null) {
                            btn.add(
                                ListTile(
                                    onTap: () {
                                        developer.log('chartalt on: $index');
                                        if (!loadTrk()) return;
                                        Pager.push(context, PageCode.chartalt);
                                    },
                                    trailing: const SizedBox.shrink(),
                                    title: Row(children: [
                                        Text('график высоты'),
                                        const Spacer(),
                                        Text(li.trk!.dtBeg),
                                        SizedBox(
                                            width: 50,
                                            child: Text(
                                                '[${li.trk!.fnum}]',
                                                style: const TextStyle(
                                                    color: Colors.black26,
                                                    fontSize: 10,
                                                ),
                                                textAlign: TextAlign.right,
                                            ),
                                        )
                                    ]),
                                )
                            );
                        }

                        return Column(children: btn);
                    },
                    separatorBuilder: (BuildContext context, int index) => index == ibeg ? const Divider() : Container(),
                );
            }
        );
    }
}
