import 'dart:async';
import 'dart:typed_data';
import 'dart:ui' as ui;
import 'dart:math';
import 'package:XdeYa/data/trkview.dart';
import 'package:flutter/services.dart';
import 'package:flutter/material.dart';
import 'package:flutter/gestures.dart';
import 'dart:developer' as developer;

import '../data/trkdata.dart';
import '../cube/painter.dart';
import '../cube/viewmatrix.dart';


class PageTrackCube extends StatelessWidget {
    PageTrackCube({ super.key });
    destroy() {
        playstop();
    }

    final _matr = ViewMatrix();
    final _notify = ValueNotifier(0);

    Timer ?_play;
    void playstart() {
        if (_play != null) return;

        void pnext() {
            if (_matr.pos+1 >= trk.data.length) {
                playstop();
                return;
            }
            final tcur = trk.data[_matr.pos];
            final tnxt = trk.data[_matr.pos+1];
            int interval = tnxt['tmoffset'] - tcur['tmoffset'];
            if (interval < 20) interval = 100;
            _play = Timer(
                Duration(milliseconds: interval-10),
                () {
                    if (_matr.pos+1 < trk.data.length) {
                        _matr.pos++;
                        _notify.value++;
                    }
                    pnext();
                }
            );
        }

        pnext();
    }
    void playstop() {
        if (_play != null) {
            _play!.cancel();
            _play = null;
        }
    }

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
                        
                                return Listener(
                                    onPointerSignal: (pointerSignal) {
                                        //pointerSignal.
                                        if (pointerSignal is PointerScrollEvent) {
                                            _matr.scaleChange(pointerSignal.scrollDelta.dy);
                                        }
                                    },
                                    child: CustomPaint(
                                        painter: CubePainter(
                                            view: _matr,
                                            onSize: (size) {
                                                _matr.size = size;
                                            }
                                        ),
                                        size: Size.infinite,
                                    )
                                );
                            }
                        )
                    )
                );
            }
        );

        return Stack(
            children: [
                cube,
                Positioned(
                    width: MediaQuery.of(context).size.width,
                    bottom: 0,
                    child: ValueListenableBuilder(
                        valueListenable: _notify,
                        builder: (BuildContext context, inf, Widget? child) {
                            final row = <Widget>[
                                FloatingActionButton(
                                    heroTag: 'info',
                                    onPressed: () {
                                        _matr.posVisible = !_matr.posVisible;
                                        if (!_matr.posVisible) {
                                            playstop();
                                        }
                                        _notify.value ++;
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
                                                if (_play != null) return;
                                                _matr.pos = value.round();
                                                _notify.value ++;
                                            },
                                        ),
                                    ),
                                    FloatingActionButton(
                                        heroTag: 'play',
                                        onPressed: () {
                                            if (_play == null) {
                                                playstart();
                                            }
                                            else {
                                                playstop();
                                            }
                                            _notify.value ++;
                                        },
                                        child: Icon(_play == null ? Icons.play_arrow : Icons.stop),
                                    ),
                                ]);
                            }
                            else {
                                row.add( const Spacer() );
                            }

                            row.add(
                                FloatingActionButton(
                                    heroTag: 'reset',
                                    onPressed: () => 
                                            _matr.reset(),
                                    child: const Icon(Icons.adjust),
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
