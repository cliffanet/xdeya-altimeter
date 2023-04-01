import 'dart:async';
import 'dart:typed_data';
import 'dart:ui' as ui;
import 'dart:math';
import 'package:XdeYa/data/trkview.dart';
import 'package:flutter/services.dart';
import 'package:flutter/material.dart';
import 'dart:developer' as developer;

import '../data/trkdata.dart';
import '../cube/painter.dart';
import '../cube/viewmatrix.dart';


class PageTrackCube extends StatelessWidget {
    PageTrackCube({ super.key });

    final _matr = ViewMatrix();
    bool _play = false;

    @override
    Widget build(BuildContext context) {
        final cube = ValueListenableBuilder(
            valueListenable: trk.notifyData,
            builder: (BuildContext context, inf, Widget? child) {
                if (trk.area == null) {
                    return const Center(
                        child: Text('нет данных')
                    );
                }

                return Center(
                    child: GestureDetector(
                        onScaleStart:   _matr.scaleStart,
                        onScaleUpdate:  _matr.scaleUpdate,
                        child: ValueListenableBuilder(
                            valueListenable: _matr.notify,
                            builder: (BuildContext context, matr, Widget? child) {
                        
                                return CustomPaint(
                                    painter: CubePainter(
                                        view: _matr,
                                        onSize: (size) {
                                            _matr.size = size;
                                        }
                                    ),
                                    size: Size.infinite,
                                );
                            }
                        )
                    )
                );
            }
        );

        final notify = ValueNotifier(0);

        return Stack(
            children: [
                cube,
                Positioned(
                    width: MediaQuery.of(context).size.width,
                    bottom: 0,
                    child: ValueListenableBuilder(
                        valueListenable: notify,
                        builder: (BuildContext context, inf, Widget? child) {
                            final row = <Widget>[
                                FloatingActionButton(
                                    onPressed: () {
                                        _matr.posVisible = !_matr.posVisible;
                                        notify.value ++;
                                    },
                                    child: Icon(_matr.posVisible ? Icons.not_interested : Icons.info),
                                ),
                            ];

                            if (_matr.posVisible) {
                                row.addAll([
                                    Expanded(
                                        child: Slider(
                                            value: _matr.pos < trk.data.length ?
                                                        _matr.pos.toDouble() :
                                                        trk.data.length-1,
                                            max: (trk.data.length-1).toDouble(),
                                            onChanged: (double value) {
                                                _matr.pos = value.round();
                                                notify.value ++;
                                            },
                                        ),
                                    ),
                                    FloatingActionButton(
                                        onPressed: () {
                                            _play = !_play;
                                            notify.value ++;
                                        },
                                        child: Icon(_play ? Icons.stop : Icons.play_arrow),
                                    ),
                                ]);
                            }
                            else {
                                row.add( Spacer() );
                            }

                            row.add(
                                FloatingActionButton(
                                    onPressed: () => 
                                            _matr.reset(),
                                    child: Icon(Icons.adjust),
                                ),
                            );

                            return Row(children: row);
                        }
                    ),
                ),
            ],
        );
    }
}
