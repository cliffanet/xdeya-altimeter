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

    @override
    Widget build(BuildContext context) {
        return ValueListenableBuilder(
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
    }
}
